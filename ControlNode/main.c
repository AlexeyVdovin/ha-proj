#ifndef __AVR__

int main()
{

    return 0;
}

#else /* __AVR__ */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "timer.h"
#include "led_display.h"
#include "sio.h"
#include "packet.h"
#include "rs485.h"
#include "ds1820.h"
#include "command.h"
#include "config.h"

static uchar mcusr;

void io_init()
{
    mcusr = MCUSR;

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
    // Port A initialization
    // Func7=Out Func6=Out Func5=Out Func4=In Func3=In Func2=In Func1=In Func0=In 
    // State7=0 State6=0 State5=0 State4=T State3=T State2=T State1=T State0=T 
    PORTA=0x00;
    DDRA=0xE0;

    // Port B initialization
    // Func7=Out Func6=In Func5=Out Func4=Out Func3=In Func2=Out Func1=Out Func0=Out 
    // State7=0 State6=P State5=0 State4=1 State3=T State2=0 State1=0 State0=0 
    PORTB=0x50;
    DDRB=0xB7;

    // Port C initialization
    // Func7=Out Func6=Out Func5=Out Func4=Out Func3=Out Func2=Out Func1=Out Func0=Out 
    // State7=0 State6=0 State5=0 State4=0 State3=0 State2=0 State1=0 State0=0 
    PORTC=0x00;
    DDRC=0xFF;

    // Port D initialization
    // Func7=Out Func6=Out Func5=Out Func4=Out Func3=In Func2=Out Func1=Out Func0=In 
    // State7=0 State6=0 State5=0 State4=0 State3=P State2=0 State1=1 State0=P 
    PORTD=0x0B;
    DDRD=0xF6;

    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: On
    // INT1 Mode: Falling Edge
    // INT2: Off
    // Interrupt on any change on pins PCINT0-7: Off
    // Interrupt on any change on pins PCINT8-15: Off
    // Interrupt on any change on pins PCINT16-23: Off
    // Interrupt on any change on pins PCINT24-31: Off
    EICRA=0x08;
    EIMSK=0x02;
    EIFR=0x02;
    PCICR=0x00;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    ADCSRB=0x00;

    // SPI initialization
    // SPI Type: Master
    // SPI Clock Rate: 3000.000 kHz
    // SPI Clock Phase: Cycle Half
    // SPI Clock Polarity: Low
    // SPI Data Order: MSB First
    SPCR=0x50;
    SPSR=0x00;
    
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

    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: 187.500 kHz
    // Mode: Normal top=FFh
    // OC0A output: Disconnected
    // OC0B output: Disconnected
    TCCR0A = 0x00;
    TCCR0B = 0x03;
    TCNT0  = 0x00;
    OCR0A  = 0x40; // LED Brightness
    OCR0B  = 0x00;

    // Timer/Counter 0 Interrupt(s) initialization
    TIMSK0=0x03;

    cfg_init();    
    timer_init();
    disp_init();
    sio_init();
    rs485_init();

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
    WDTCSR=0x1E;
    asm (" NOP" ); 
    WDTCSR=0x0E;
    
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
        		        ds1820_req = get_time() + (cfg_ds1820_period()/ds1820count - 80);
        		        ds1820_ack = 0;
        		        if(++d >= ds1820count) d = 0;
		                r = 5;
        		    }
                }    	
            }
    	} 
    	else if(ds1820_req < get_time() && ds1820scan() == 0)
    	{
    	    ds1820_req = get_time() + cfg_ds1820_period();
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
