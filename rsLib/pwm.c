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

}

ushort pwm_read(uchar ch)
{
    ushort pwm = 0;

    if(ch == 1)
    {
        pwm = PWM_CH1;
    }
    else if(ch == 2)
    {
        pwm = PWM_CH2;
    }
    return pwm;
}

// TODO: Check if clock turned OFF, then use PORTB directly
void pwm_set(uchar ch, ushort pwm)
{
    if(ch == 1)
    {
        PWM_CH1 = pwm;
    }
    else if(ch == 2)
    {
        PWM_CH2 = pwm;
    }
}

/* Clock Select Description
    0 - Off
    1 - 1/1
    2 - 1/8
    3 - 1/64
    4 - 1/256
    5 - 1/1024
*/
void pwm_freq(uchar d)
{
    PWM_FREQ(d);
}

#endif /* __AVR__ */
