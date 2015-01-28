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

uchar data[] = { DATA_ID1, DATA_ID2, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 };

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

uchar cmd_read(uchar len, uchar* data)
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

uchar cmd_write(uchar len, uchar* data)
{
    uchar res = 01;
    switch(data[1])
    {
    case 80:
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
                    memcpy(buff + wr_pos, data+2, len-2);
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
                ushort i, calc = 0;
                ushort start = data[2] + (data[3] << 8);
                ushort end = data[4] + (data[5] << 8);
                ushort crc = data[6] + (data[7] << 8);
                for(i = start; i <= end; ++i) calc = _crc_xmodem_update(calc, pgm_read_byte(i));
                if(crc == calc) res = 00; // CRC OK
                else            res = 04; // CRC missmatch
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

packet_t* cmd_proc(packet_t* pkt)
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


int main()
{
    static uchar fs = S_IDLE;
    uint i;
    packet_t* pkt;

#if 0    
    mcucsr = MCUCSR;
    // Reset Source checking
    if (MCUCSR & 1)
       {
       // Power-on Reset
       MCUCSR=0;
       // Place your code here

       }
    else if (MCUCSR & 2)
       {
       // External Reset
       MCUCSR=0;
       // Place your code here

       }
    else if (MCUCSR & 4)
       {
       // Brown-Out Reset
       MCUCSR=0;
       // Place your code here

       }
    else
       {
       // Watchdog Reset
       MCUCSR=0;
       // Place your code here

       };
#endif

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
    TCCR0=0x00;
    TCNT0=0x00;

    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: SYSTEM_CLOCK/1
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
    
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 11.719 kHz
    // Mode: CTC top=OCR2
    // OC2 output: Disconnected
    ASSR  = 0x00;
    TCCR2 = 0x0F;
    TCNT2 = 0x00;
    OCR2  = 0x75;
    
    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    MCUCR=0x00;

    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK=0x80;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    SFIOR=0x00;

    cfg_init();
    sio_init();

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
    WDTCR=0x1E;
    asm (" NOP" ); 
    WDTCR=0x0E;
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_2S);

    // Global enable interrupts
    sei();

 	data[3] = cfg_node_id();

 	for (;;)
    {
    	wdt_reset();

        pkt = rs485_rx_packet();
        if(pkt && pkt->to != 0 && pkt->to != cfg_node_id()) pkt = NULL;
        if(pkt) pkt = cmd_proc(pkt);
        if(pkt) rs485_tx_packet(pkt);
        
        if(fs == S_IDLE && wr_pos >= SPM_PAGESIZE)
        {
            boot_page_erase(wr_page);
            fs = S_FLASH_EN;
        }
        
        if(fs == S_FLASH_EN && !boot_spm_busy())
        {
            for(i = 0; i < SPM_PAGESIZE; i += 2)
            {
                // Set up little-endian word.
                ushort w = buff[i] + (buff[i+1] << 8);
                boot_page_fill(wr_page + i, w);
            }
            wr_pos -= SPM_PAGESIZE;
            memmove(buff, buff+i, wr_pos);
            boot_page_write (wr_page); // Store buffer in flash page.
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
