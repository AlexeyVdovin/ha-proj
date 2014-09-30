#ifndef _TIMER_H_
#define _TIMER_H_

#include "defines.h"

void timer_init();

/* Return count of 10ms ticks */
long get_time();

/* Delay for n*10ms */
void delay_t(uchar t);

/* Delay for n sec */
void delay_s(uchar s);

#endif /* _TIMER_H_ */
