#ifndef __AVR__

#include "led_display.h"

volatile uchar disp[4];
volatile uchar buttons;

void disp_init()
{

}

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "led_display.h"

static const uchar digits[] = { 0x6f, 0x60, 0xa7, 0xe6, 0xe8, 0xce, 0xcf, 0x62, 0xef, 0xee, 0x80 /* - */, 0x00, 0xaa};

// Char position on display
static volatile uchar pos;  
static volatile uchar bn[4];

volatile uchar disp[4];
volatile uchar buttons;


// Timer 0 overflow interrupt service routine
ISR(TIMER0_OVF_vect)
{
    if(++pos == 4) pos = 0;
    PORTD &= 0x0F;
    PORTC = disp[pos];
    PORTD = (PORTD & 0x0F)|(0x10<<pos);
}

// Timer 0 output compare A interrupt service routine
ISR(TIMER0_COMPA_vect)
{
    if(PORTB & 0x40) /* PINB.3 */
    {
        if(++bn[pos] == PRESS_DELAY)
        {
            buttons |= (1<<pos);
        }
        else if(bn[pos] == LONG_PRESS_DELAY)
        {
            buttons |= (0x10<<pos);
        }
        else if(bn[pos] > LONG_PRESS_DELAY)
        {
            bn[pos] = LONG_PRESS_DELAY;
        }
    }
    else
    {
        if(bn[pos] > PRESS_DELAY) 
            bn[pos] = PRESS_DELAY;
        if(--bn[pos] < RELEASE_DELAY)
        {
            bn[pos] = RELEASE_DELAY;
            buttons &= ~(0x11<<pos);
        }
    }
    PORTD &= 0x0F;
}

void disp_init()
{
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
    
    disp[0] = 0x00;
    disp[1] = 0x00;
    disp[2] = 0x00;
    disp[3] = 0x00;
}

void disp_digit(uchar p, uchar n)
{
    disp[p] = digits[n];
}

void disp_set_brightness(uchar c)
{
    OCR0A = c; // LED Brightness
}

#endif /* __AVR__ */
