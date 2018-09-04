#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "timer.h"
#include "sio.h"
#include "adc.h"
#include "config.h"
#include "twi_slave.h"

static uchar mcucsr;

void io_init()
{
    mcucsr = MCUSR;

    // Input/Output Ports initialization
    // Port B initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=Out Func1=Out Func0=In 
    // State7=T State6=T State5=T State4=T State3=T State2=0 State1=0 State0=T 
    PORTB = 0x38;
    DDRB = 0x07;

    // Port C initialization
    // Func6=In Func5=In Func4=In Func3=In Func2=In Func1=Out Func0=In 
    // State6=T State5=T State4=T State3=T State2=T State1=0 State0=T 
    PORTC = 0x08;
    DDRC = 0x00;

    // Port D initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=Out Func2=Out Func1=Out Func0=In 
    // State7=T State6=T State5=T State4=T State3=1 State2=0 State1=1 State0=P 
    PORTD = 0x04;
    DDRD = 0xE4;

    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: Timer 0 Stopped
    // Mode: Normal top=FFh
    // OC0A output: Disconnected
    // OC0B output: Disconnected
    TCCR0A = 0x00;
    TCCR0B = 0x00;
    TCNT0 = 0x00;
    OCR0A = 0x00;
    OCR0B = 0x00;    
    // Timer/Counter 0 Interrupt(s) initialization
    TIMSK0 = 0x00;

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
    TCCR1A = 0x00;
    TCCR1B = 0x00;
    TCNT1H = 0x00;
    TCNT1L = 0x00;
    ICR1H = 0x00;
    ICR1L = 0x00;
    OCR1AH = 0x00;
    OCR1AL = 0x00;
    OCR1BH = 0x00;
    OCR1BL = 0x00;
    // Timer/Counter 1 Interrupt(s) initialization
    TIMSK1 = 0x00;
   
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 11.719 kHz
    // Mode: CTC top=OCR2A
    // OC2A output: Disconnected
    // OC2B output: Disconnected
    ASSR = 0x00;
    TCCR2A = 0x02;
    TCCR2B = 0x07;
    TCNT2 = 0x00;
    OCR2A = 0x75;
    OCR2B = 0x00;
    // Timer/Counter 2 Interrupt(s) initialization
    TIMSK2 = 0x02;
    
    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // Interrupt on any change on pins PCINT0-7: Off
    // Interrupt on any change on pins PCINT8-14: Off
    // Interrupt on any change on pins PCINT16-23: Off
    EICRA = 0x00;
    EIMSK = 0x00;
    PCICR = 0x00;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR = 0x80;
    ADCSRB = 0x00;

    cfg_init();
    adc_init();
    sio_init();

    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/1024k
  	wdt_reset();
    WDTCSR = 0x39;
    asm (" NOP" ); 
    WDTCSR = 0x29;
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_2S);

    // Global enable interrupts
    sei();
}

#include <stdio.h>

static int uart_putchar(char c, FILE *stream)
{
  return sio_putchar(c);
}

void led_red_on()
{
    PORTD |= 0x20;
}

void led_red_off()
{
    PORTD &= ~ 0x20;
}

void led_green_on()
{
    PORTD |= 0x40;
}

void led_green_off()
{
    PORTD &= ~ 0x40;
}


static FILE my_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

int main()
{
    long s = get_time() + 100;
    io_init();
    twi_init(0x16);
    
    PORTD |=  0x04;

 	for (;;)
    {
        if(timer_check(s))
        {
            // printf("Hello !\n");
            s = get_time() + 100;
        }
    	wdt_reset();
        sleep_mode();
    }
    return (0);
}
