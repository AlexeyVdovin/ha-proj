#ifndef __AVR__

#include <unistd.h>
#include <sys/time.h>

#include "timer.h"


void timer_init()
{
    
}

long get_time()
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

static volatile long timer = 0;

ISR(TIMER_ISR)
{
#ifdef _USE_TIME_H_
    static uchar t = 100;
    
    if(--t == 0)
    {
        t = 100;
        system_tick();
    }
#endif
    ++timer;
}

void timer_init()
{

}

long get_time()
{
    cli();
    long t = timer;
    sei();
    return t;
}

inline bool timeout_expired(long timeout)
{
  long t = get_time(); 
  return ((t - timeout) <  (1UL << 31) && (timeout <= t)) ? true : false;
}

void delay_t(uchar t)
{
    long s = get_time() + t;
    do
    {
        wdt_reset(); 
        sleep_mode();
    } while(!timeout_expired(s));
}

void delay_s(uchar s)
{
    long t = get_time() + s*100;
    do
    {
        wdt_reset(); 
        sleep_mode();
    } while(!timeout_expired(t));
}

#endif /* __AVR__ */

