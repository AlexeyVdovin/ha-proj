#ifndef _SW_PWM_H_
#define _SW_PWM_H_

#include "defines.h"

void swpwm_init();

ushort swpwm_read(uchar ch);

void swpwm_set(uchar ch, ushort pwm);

/* Clock Select Description
    0 - Off
    1 - 1/1/1024
    2 - 1/8/1024
    3 - 1/64/1024
    4 - 1/256/1024
    5 - 1/1024/1024
*/
void swpwm_freq(uchar d);


#endif /* _SW_PWM_H_ */
