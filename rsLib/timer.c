#ifndef __AVR__

#include <unistd.h>
#include <sys/time.h>

#include "timer.h"

void timer_init()
{
    
}

ulong get_time()
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

#else

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "timer.h"

volatile ulong time = 0;

ISR(TIMER_ISR)
{
    ++time;
}

void timer_init()
{
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 11.719 kHz
    // Mode: CTC top=OCR2
    // OC2 output: Disconnected
    TIMER_ASSR = 0x00;
    TIMER_TCCR = 0x0F;
    TIMER_TCNT = 0x00;
    TIMER_OCR  = 0x75;
}

ulong get_time()
{
    cli();
    ulong t = time;
    sei();
    return t;
}

void delay_t(uchar t)
{
    ulong s = get_time() + t;
    do
    {
        wdt_reset(); 
//        sleep_mode();
    } while(s > get_time());
}

void delay_s(uchar s)
{
    ulong t = get_time() + s*100;
    do
    {
        wdt_reset(); 
        sleep_mode();
    } while(t > get_time());
}

#endif
