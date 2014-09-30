#ifndef __AVR__

#include "sw_pwm.h"

void swpwm_init()
{

}

ushort swpwm_read(uchar ch)
{
    return 0;
}

void swpwm_set(uchar ch, ushort pwm)
{

}

void swpwm_freq(uchar d)
{

}

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "sw_pwm.h"

static volatile ushort pwm_count;
static volatile ushort pwm_ch[SW_PWM_CHS];

ISR(TIMER1_OVF_vect)
{
    uchar i;
    if(--pwm_count)
    {
        for(i = 0; i < SW_PWM_CHS; ++i)
        {
            if(pwm_ch[i] == pwm_count) SW_PWM_CH_ON(i);
        }
    }
    else
    {
        pwm_count = 0x400;
        for(i = 0; i < SW_PWM_CHS; ++i)
        {
            if(pwm_ch[i] == 0x3FF) SW_PWM_CH_ON(i);
            else SW_PWM_CH_OFF(i);
        }
    }
}

void swpwm_init()
{
    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: SYSTEM_CLOCK/64
    // Mode: Fast PWM top=03FFh
    // OC1A output: Off.
    // OC1B output: Off.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer 1 Overflow Interrupt: On
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A=0x03;
    TCCR1B=0x0B;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;

    memset((void*)pwm_ch, 0, sizeof(pwm_ch));
    pwm_count = 1;
}

ushort swpwm_read(uchar ch)
{
    return (ch < SW_PWM_CHS) ? pwm_ch[ch] : 0;
}

void swpwm_set(uchar ch, ushort pwm)
{
    if(ch < SW_PWM_CHS) pwm_ch[ch] = pwm;
}

void swpwm_freq(uchar d)
{
    TCCR1B = (TCCR1B & 0xF8) | (d & 0x07);
}

#endif /* __AVR__ */

