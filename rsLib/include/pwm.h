#ifndef _PWM_H_
#define _PWM_H_

#include "defines.h"

void pwm_init();

ushort pwm_read(uchar ch);

void pwm_set(uchar ch, ushort pwm);

/* Clock Select Description
    0 - Off
    1 - 1/1
    2 - 1/8
    3 - 1/64
    4 - 1/256
    5 - 1/1024
*/
void pwm_freq(uchar d);

#endif /* _PWM_H_ */
