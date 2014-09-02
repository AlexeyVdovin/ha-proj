#ifndef _PWM_H_
#define _PWM_H_

#include "defines.h"

void pwm_init();

ushort pwm_read(uchar ch);

void pwm_set(uchar ch, ushort pwm);

#endif /* _PWM_H_ */
