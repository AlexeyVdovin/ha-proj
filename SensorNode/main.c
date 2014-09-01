#ifndef __AVR__

#include <stdio.h>

#include "timer.h"
#include "packet.h"
#include "udp.h"
#include "adc.h"

#include <unistd.h>

static void pkt_dump(packet_t* pkt)
{
    int i;
    printf("Pkt: %02X%02X %02X -> %02X -> %02X  (%02X) %02X [%d]:", pkt->id[0], pkt->id[1], pkt->from, pkt->via, pkt->to, pkt->flags, pkt->seq, pkt->len);
    for(i = 0; i < pkt->len; ++i) { printf(" %02X", pkt->data[i]); }
    printf(" %02X%02X\n", pkt->data[pkt->len], pkt->data[pkt->len+1]);
}

int main(int argc, char** argv)
{
    int n = 0;
    timer_init();
    udp_init();
    adc_init();
    
    uchar data[] = { DATA_ID1, DATA_ID2, 0x00, 0x01, 0x00, 0x00, 0xBE, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00 };
    pid_t pid = getpid();
    data[3] = (uchar)(pid&0xFF);
    
    do
    {
        packet_t* pkt = udp_rx_packet();
        if(pkt)
        {
            pkt_dump(pkt);
        }
        delay_t(1);
        if(++n > 200)
        {
            n = 0;
            udp_tx_packet((packet_t*)data);
        }
            
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
#include "command.h"
#include "config.h"

static uchar mcucsr;

void io_init()
{
    mcucsr = MCUCSR;

#if 0    
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
    // Clock value: Timer 1 Stopped
    // Mode: Normal top=FFFFh
    // OC1A output: Discon.
    // OC1B output: Discon.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer 1 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A=0x00;
    TCCR1B=0x00;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;
    
    cfg_init();
    timer_init();
    adc_init();
    sio_init();
    rs485_init();

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

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
    WDTCR=0x1E;
    asm (" NOP" ); 
    WDTCR=0x0E;
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_2S);

    // Global enable interrupts
    sei();
}

uchar data[] = { DATA_ID1, DATA_ID2, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 };

int main()
{
    uint i = 0;
    uchar d = 0, n = 0;
    packet_t* pkt;

    io_init ();
	ulong ds1820_req = 0;
    n = ds1820scan();

    for (;;)
    {
    	wdt_reset();

        if(ds1820_req == 0)
        {
		    ds1820queryprobe(d);
		    ds1820_req = get_time() + 80; // 800ms
		}
		else if(ds1820_req < get_time())
		{
		    uchar r = 5;
		    while(ds1820updateprobe(d) == 0)
		    {
        		if(--r == 0) break;
        		delay_t(1); // 10ms
		    }
		    ds1820_req = 0;
		    if(++d >= n) d = 0;
        }    	
    	
        pkt = rs485_rx_packet();
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
            pkt->data[0] = n;
            rs485_tx_packet(pkt);
        }
    }
    return (0);
}

#endif /* __AVR__ */

