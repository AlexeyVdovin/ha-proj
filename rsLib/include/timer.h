#ifndef _TIMER_H_
#define _TIMER_H_

#include "defines.h"

#ifdef _USE_TIME_H_
#include <time.h>
#endif

void timer_init();

/* Return count of 10ms ticks */
short get_time();

/* Delay for n*10ms */
void delay_t(uchar t);

/* Delay for n sec */
//void delay_s(uchar s);

#endif /* _TIMER_H_ */
