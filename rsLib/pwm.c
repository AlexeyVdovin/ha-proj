#ifndef __AVR__

#include "pwm.h"

void pwm_init()
{
    
}

ushort pwm_read(uchar ch)
{
    return 0;
}

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "pwm.h"
#include "timer.h"

void pwm_init()
{
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
}

ushort pwm_read(uchar ch)
{
    ushort pwm = 0;

    if(ch == 0)
    {
        pwm = OCR1A;
    }
    else if(ch == 1)
    {
        pwm = OCR1B;
    }
    return pwm;
}

void pwm_set(uchar ch, ushort pwm)
{
    if(ch == 0)
    {
        OCR1A = pwm;
    }
    else if(ch == 1)
    {
        OCR1B = pwm;
    }
}


#endif /* __AVR__ */
