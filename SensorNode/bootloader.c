#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>

#include <string.h> // for memcpy

#include "timer.h"
#include "sio.h"
#include "packet.h"
#include "rs485.h"

#include "config.h"

static uchar mcucsr;

enum
{
    S_IDLE = 0,
    S_FLASH_EN,
    S_FLASH_WR,
    S_FLASH_OK
};

#define WR_BUFF_SIZE    256
#define DEV_ID          0xA1

static uchar status = 0;
static uchar buff[WR_BUFF_SIZE];
static ushort wr_page;
static ushort wr_pos;

static ushort calc_crc(ushort addr, ushort end)
{
    ushort crc = 0;
    while(addr <= end) { ++addr; crc = _crc_xmodem_update(crc, pgm_read_byte(addr)); }
    return crc;
}

static uchar cmd_read(uchar len, uchar* data)
{
    switch(data[1])
    {
    case 00:
        data[1] = 00;
        data[2] = DEV_ID; // Defined in config.h
        data[3] = SPM_PAGESIZE;
        data[4] = status;
        data[5] = wr_page; // Progress
        len = 6;
        break;
    default:
        return 0;
    }
    return len;
}

static uchar cmd_write(uchar len, uchar* data)
{
    uchar res = 01;
    switch(data[1])
    {
    case 0x80:
        {   // Switch to flash mode
            if(data[2] == 02 && status == S_IDLE)
            {
                status = S_FLASH_EN;
                wr_page = 0;
                wr_pos = 0;
                res = 00;
            }
            len = 2;
        }
        break;
    case 0xC0: // Enable flashing
        {
            if(status == S_FLASH_EN)
            {
                status = S_FLASH_WR;
                res = 00;
            }
            else
            {
                status = S_IDLE;
            }
            len = 2;
        }
        break;
    case 0xC3: // Write data
        {
            if(status == S_FLASH_WR && len > 2)
            {
                if(wr_pos + len - 2 < WR_BUFF_SIZE)
                {
                    memmove(buff + wr_pos, data+2, len-2);
                    wr_pos += len-2;
                    res = 00;
                }
                else
                {
                    res = 03; // Overflow, retry latter
                }
            }
            else
            {
                status = S_IDLE;
            }
            len = 2;
        }
        break;
    case 0xC4: // Calc CRC
        {
            if(status == S_FLASH_WR || status == S_IDLE)
            {
                ushort end   = (ushort)(data[2]) + (data[3] << 8);
                ushort crc   = (ushort)(data[4]) + (data[5] << 8);
                if(crc == calc_crc(0, end))
                {
                    cfg_set_flash_size(end);
                    cfg_set_flash_crc(crc);
                    res = 00; // CRC OK
                }
                else res = 04; // CRC missmatch
            }
            len = 2;
        }
        break;
    default:
        return 0;
    }
    data[1] = res;
    return len;
}

static packet_t* cmd_proc(packet_t* pkt)
{
    uchar len = 0;
        
    pkt->to = pkt->from;
    pkt->from = cfg_node_id();
    pkt->via = 0;
    
    if(pkt->data[0] == 0x02 && pkt->len > 1) len = cmd_read(pkt->len, pkt->data);
    if(pkt->data[0] == 0x04 && pkt->len > 1) len = cmd_write(pkt->len, pkt->data);

    pkt->data[0] = 0x01;
    if(!len)
    {
        pkt->data[1] = 0x02; /* Not Implemented */
        len = 2;
    }
    pkt->len = len;
    return pkt;
}

static void (*jump_to_app)(void) = 0x0000;

int main()
{
    static uchar fs = S_IDLE;
    uint i;
    packet_t* pkt;
    mcucsr = MCUSR;

    // Input/Output Ports initialization
    // Port B initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=Out Func1=Out Func0=In 
    // State7=T State6=T State5=T State4=T State3=T State2=0 State1=0 State0=T 
    PORTB=0x00;
    DDRB=0x06;

    // Port C initialization
    // Func6=In Func5=In Func4=In Func3=In Func2=In Func1=Out Func0=In 
    // State6=T State5=T State4=T State3=T State2=T State1=0 State0=T 
    PORTC=0x00;
    DDRC=0x02;

    // Port D initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=Out Func2=Out Func1=Out Func0=In 
    // State7=T State6=T State5=T State4=T State3=1 State2=0 State1=1 State0=P 
    PORTD=0x0B;
    DDRD=0x0E;

    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: Timer 0 Stopped
    // Mode: Normal top=FFh
    // OC0A output: Disconnected
    // OC0B output: Disconnected
    TCCR0A=0x00;
    TCCR0B=0x00;
    TCNT0=0x00;
    OCR0A=0x00;
    OCR0B=0x00;    
    // Timer/Counter 0 Interrupt(s) initialization
    TIMSK0=0x00;

    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: 12000.000 kHz
    // Mode: Fast PWM top=03FFh
    // OC1A output: Non-Inv.
    // OC1B output: Non-Inv.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer 1 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A=0xA3;
    TCCR1B=0x09;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;
    // Timer/Counter 1 Interrupt(s) initialization
    TIMSK1=0x00;
    
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 11.719 kHz
    // Mode: CTC top=OCR2A
    // OC2A output: Disconnected
    // OC2B output: Disconnected
    ASSR=0x00;
    TCCR2A=0x02;
    TCCR2B=0x07;
    TCNT2=0x00;
    OCR2A=0x75;
    OCR2B=0x00;
    // Timer/Counter 2 Interrupt(s) initialization
    TIMSK2=0x02;
    
    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // Interrupt on any change on pins PCINT0-7: Off
    // Interrupt on any change on pins PCINT8-14: Off
    // Interrupt on any change on pins PCINT16-23: Off
    EICRA=0x00;
    EIMSK=0x00;
    PCICR=0x00;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    ADCSRB=0x00;

    cfg_init();
    sio_init();

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
  	wdt_reset();
    WDTCSR=0x39;
    asm (" NOP" ); 
    WDTCSR=0x29;
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_2S);
    
    if(calc_crc(0, cfg_flash_size()) == cfg_flash_crc())
    {
        MCUSR = mcucsr;
        jump_to_app();
    }

    // Move interrupt vectors to bootloader section
    MCUCR = (1<<IVCE);
    MCUCR = (1<<IVSEL);
    
    // Global enable interrupts
    sei();

 	for (;;)
    {
    	wdt_reset();

        pkt = rs485_rx_packet();
        if(pkt && pkt->to != 0 && pkt->to != cfg_node_id()) pkt = NULL;
        if(pkt) pkt = cmd_proc(pkt);
        if(pkt) rs485_tx_packet(pkt);
        
        ushort addr = wr_page*SPM_PAGESIZE;
        
        if(fs == S_IDLE && wr_pos >= SPM_PAGESIZE)
        {
            boot_page_erase(addr);
            fs = S_FLASH_EN;
        }
        
        if(fs == S_FLASH_EN && !boot_spm_busy())
        {
            for(i = 0; i < SPM_PAGESIZE; i += 2)
            {
                // Set up little-endian word.
                ushort w = buff[i] + (buff[i+1] << 8);
                boot_page_fill(addr + i, w);
            }
            wr_pos -= i;
            memmove(buff, buff+i, wr_pos);
            boot_page_write (addr);
            fs = S_FLASH_WR;
        }
        
        if(fs == S_FLASH_WR && !boot_spm_busy())
        {
            ++wr_page;
            fs = S_IDLE;
        }
    	
        sleep_mode();
    }
    return 0;
}

