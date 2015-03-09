#ifndef __AVR__

#include <stdio.h>

#include "timer.h"
#include "packet.h"
#include "udp.h"
#include "pwm.h"
#include "adc.h"

#include <unistd.h>
#include <string.h>

static uchar node_id = 0x01;
static uchar* src = NULL;
static ushort data_len = 0;

static void pkt_dump(char* str, packet_t* pkt)
{
    int i;
    printf("%s Pkt: %02X%02X %02X -> %02X -> %02X  (%02X) %02X [%d]:", str, pkt->id[0], pkt->id[1], pkt->from, pkt->via, pkt->to, pkt->flags, pkt->seq, pkt->len);
    for(i = 0; i < pkt->len; ++i) { printf(" %02X", pkt->data[i]); }
    printf(" %02X%02X\n", pkt->data[pkt->len], pkt->data[pkt->len+1]);
}

/* Calculating XMODEM CRC-16 in 'C'
   ================================
   Reference model for the translated code */

#define poly 0x1021

/* On entry, addr=>start of data
             num = length of data
             crc = incoming CRC     */
static unsigned short crc16(char *addr, int num, unsigned int crc)
{
    int i;

    for(; num>0; num--)                      /* Step through bytes in memory */
    {
        crc = crc ^ (*addr++ << 8);          /* Fetch byte from memory, XOR into CRC top byte*/
        for (i=0; i<8; i++)                  /* Prepare to rotate 8 bits */
        {
            crc = crc << 1;                  /* rotate */
            if (crc & 0x10000)               /* bit 15 was set (now bit 16)... */
                crc = (crc ^ poly) & 0xFFFF; /* XOR with XMODEM polynomic */
                                             /* and ensure CRC remains 16-bit value */
        }                                    /* Loop for 8 bits */
    }                                        /* Loop until num=0 */
    return (unsigned short)crc;              /* Return updated CRC */
}

enum
{
    ST_IDLE = 0,
    ST_FF_EN,
    ST_FF_WR,
    ST_FF_CHK,
    ST_FF_OK
};

uchar pkt_buff[32];

packet_t* process_pkt(packet_t* pkt)
{
    static uchar seq = 0x01;
    static uchar state = ST_IDLE;
    static uchar to = 0;
    static uchar req = 0;
    static ushort pos = 0;
    static uchar dev_status = 0;
    
    uchar data[] = { DATA_ID1, DATA_ID2, to, node_id, 0x00, 0x00, seq, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    memcpy(pkt_buff, data, sizeof(data));
    
    do
    {
        if(pkt == NULL && state == ST_IDLE)
        {
            pkt = (packet_t*)pkt_buff;
            pkt->data[0] = 0x02; // READ
            pkt->data[1] = 0x00; // STATUS
            pkt->len = 2;
            req = pkt->data[1];      
            break;
        }
        
        if(pkt == NULL || pkt->seq != seq) { pkt->len = 0; break; }
        
        if(req == 00 && pkt->data[0] == 0x01 && pkt->data[1] == 0x00)
        {
            char* status[] = { "IDLE", "INIT", "FALSH", "OK" };
            dev_status = pkt->data[4]&3;
            to = pkt->from;
            printf("Status: Dev: %02x, Pg.size: %d, %s, page: %d\n", pkt->data[2], pkt->data[3], status[dev_status], pkt->data[5]);
        }
        
        if(state == ST_IDLE && req == 00)
        {
            if(dev_status != 0)
            {
                printf("Error: Invalid device status!\n");
                pkt = NULL;
                break;
            }
            // Init flash
            pkt = (packet_t*)pkt_buff;
            pkt->data[0] = 0x04;  // WRITE
            pkt->data[1] = 0x80;  // FF_INIT
            pkt->data[2] = 0x02;  // magic
            pkt->len = 3;
            req = pkt->data[1];      
            break;
        }

        if(state == ST_IDLE && req == 0x80)
        {
            if(pkt->data[1] == 0x00)
            {
                pkt = (packet_t*)pkt_buff;
                pkt->data[0] = 0x02; // READ
                pkt->data[1] = 0x00; // STATUS
                pkt->len = 2;
                req = pkt->data[1];      
                state = ST_FF_EN;
                break;
            }
            else
            {
                printf("Error: Invalid device response!\n");
                pkt = NULL;
                break;
            }
        }
        
        if(state == ST_FF_EN && req == 00)
        {
            if(dev_status != 1)
            {
                printf("Error: Invalid device status!\n");
                pkt = NULL;
                break;
            }
            // Write flash
            pkt = (packet_t*)pkt_buff;
            pkt->data[0] = 0x04;  // WRITE
            pkt->data[1] = 0xC0;  // FF_WRITE
            pkt->len = 2;
            req = pkt->data[1];      
            break;
        }
        
        if(state == ST_FF_EN && req == 0xC0)
        {
            if(pkt->data[1] == 0x00)
            {
                pkt = (packet_t*)pkt_buff;
                pkt->data[0] = 0x02; // READ
                pkt->data[1] = 0x00; // STATUS
                pkt->len = 2;
                req = pkt->data[1];      
                state = ST_FF_WR;
                break;
            }
            else
            {
                printf("Error: Invalid device response!\n");
                pkt = NULL;
                break;
            }
        }

        if(state == ST_FF_WR && req == 00)
        {
            if(dev_status != 2)
            {
                printf("Error: Invalid device status!\n");
                pkt = NULL;
                break;
            }
            // Write page
            pkt = (packet_t*)pkt_buff;
            pkt->data[0] = 0x04;  // WRITE
            pkt->data[1] = 0xC3;  // FF_PAGE
            memcpy(pkt->data+2, src+pos, MAX_DATA_LEN-2);
            pkt->len = MAX_DATA_LEN;
            req = pkt->data[1];
            break;
        }

        if(state == ST_FF_WR && req == 0xC3)
        {
            if(pkt->data[1] == 0x00)
            {
                pos += MAX_DATA_LEN-2;
                pkt = (packet_t*)pkt_buff;
                pkt->data[0] = 0x02; // READ
                pkt->data[1] = 0x00; // STATUS
                pkt->len = 2;
                req = pkt->data[1];
                if(pos >= data_len) state = ST_FF_CHK;
                break;
            }
            else if(pkt->data[1] == 0x03) // Busy
            {
                pkt = (packet_t*)pkt_buff;
                pkt->data[0] = 0x02; // READ
                pkt->data[1] = 0x00; // STATUS
                pkt->len = 2;
                req = pkt->data[1];
                break;
            }
            else
            {
                printf("Error: Invalid device response!\n");
                pkt = NULL;
                break;
            }
        }
        
        if(state == ST_FF_CHK && req == 00)
        {
            // Calc CRC
            ushort crc = crc16(src, data_len, 0);
            pkt = (packet_t*)pkt_buff;
            pkt->data[0] = 0x04;  // WRITE
            pkt->data[1] = 0xC4;  // FF_CRC
            pkt->data[2] = (uchar)(data_len & 0x00FF);
            pkt->data[3] = (uchar)((data_len >> 8) & 0x00FF);
            pkt->data[4] = (uchar)(crc & 0x00FF);
            pkt->data[5] = (uchar)((crc >> 8) & 0x00FF);
            pkt->len = 6;
            req = pkt->data[1];
            break;
        }

        if(state == ST_FF_CHK && req == 0xC4)
        {
            if(pkt->data[1] == 0x00)
            {
                printf("CRC - OK.\n");
                pkt = (packet_t*)pkt_buff;
                pkt->data[0] = 0x02; // READ
                pkt->data[1] = 0x00; // STATUS
                pkt->len = 2;
                req = pkt->data[1];      
                state = ST_FF_OK;
                break;
            }
            else
            {
                printf("Error: Invalid device response!\n");
                pkt = NULL;
                break;
            }
        }
        
    }  while(0);

    return pkt;
}

uchar f_data[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77
};

int main(int argc, char** argv)
{
    int n = 0;
    timer_init();
    udp_init();
    
    src = f_data;
    data_len = sizeof(f_data);
    
    packet_t* p = process_pkt(NULL);
    
    do
    {
        if(p != NULL)
        {
            pkt_dump("to udp ", p);
            udp_tx_packet(p);
        }
        p = udp_rx_packet();
        if(p && p->from != node_id)
        {
            pkt_dump("from udp ", p);
            p = process_pkt(p);
            continue;
        }
        p = NULL;
        usleep(1000); // 1 ms
    } while(1);
    
	return 0;
}
#else /* __AVR__ */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "timer.h"
#include "sio.h"
#include "packet.h"
#include "rs485.h"
#include "ds1820.h"
#include "adc.h"
#include "pwm.h"
#include "command.h"
#include "config.h"

static uchar mcucsr;

void io_init()
{
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
    pwm_init();
    adc_init();
    sio_init();

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
  	wdt_reset();
    WDTCSR=0x39;
    asm (" NOP" ); 
    WDTCSR=0x29;
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_2S);

    // Global enable interrupts
    sei();
}

uchar data[] = { DATA_ID1, DATA_ID2, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 };

int main()
{
    uint i = 0;
    uchar d = 0;
    uchar r = 5;
    packet_t* pkt;

    io_init ();
	long ds1820_req = 0;
	uchar ds1820_ack = 0;

 	data[3] = cfg_node_id();

 	for (;;)
    {
    	wdt_reset();
        if(ds1820count > 0)
        {
            if(ds1820_req < get_time())
            {
                if(ds1820_ack == 0)
                {
		            ds1820queryprobe(d);
		            ds1820_req = get_time() + 80; // 800ms
		            ds1820_ack = 1;
		        }
		        else
		        {
		            if(ds1820updateprobe(d) == 0 && --r > 0)
		            {
		                ds1820_req = get_time() + 1; // Retry in 10ms
		            }
		            else
		            {
		                if(r == 0) { ds1820probes[d].flags &= ~DS1820FLAG_SLOTINUSE; } 
//        		        ds1820_req = get_time() + (cfg_ds1820_period()/ds1820count - 80);
        		        ds1820_ack = 0;
        		        if(++d >= ds1820count) d = 0;
		                r = 5;
        		    }
                }    	
            }
    	} 
    	else if(ds1820_req < get_time() && ds1820scan() == 0)
    	{
//    	    ds1820_req = get_time() + cfg_ds1820_period();
    	}
        pkt = rs485_rx_packet();
        if(pkt && pkt->to != 0 && pkt->to != cfg_node_id()) pkt = NULL;
        if(pkt) pkt = cmd_proc(pkt);
        if(pkt)
        {
            rs485_tx_packet(pkt);
            i = 0;
        } 
        sleep_mode();
       
        if(++i > 20000)
        {
            i = 0;
            pkt = (packet_t*) data;
            pkt->from = cfg_node_id();
            pkt->data[0] = ds1820count;
            rs485_tx_packet(pkt);
        }
    }
    return (0);
}

#endif /* __AVR__ */

