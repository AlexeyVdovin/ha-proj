#ifndef __AVR__

#include <unistd.h>
#include <sys/time.h>

#include "timer.h"

void timer_init()
{
    
}

short get_time()
{
    struct timeval  tv;
    gettimeofday(&tv, NULL);

    return ((tv.tv_sec)*1000L + (tv.tv_usec)/1000)/10;
}

void delay_t(uchar t)
{
    usleep(t*10000);
}

void delay_s(uchar s)
{
    usleep(s*1000*1000);
}

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "timer.h"

static volatile short timer = 0;

ISR(TIMER_ISR)
{
    ++timer;
}

short get_time()
{
    cli();
    short t = timer;
    sei();
    return t;
}

#endif /* __AVR__ */

