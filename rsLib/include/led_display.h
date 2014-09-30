#ifndef _LED_DISPLAY_H_
#define _LED_DISPLAY_H_

#include "defines.h"

extern volatile uchar disp[4];
extern volatile uchar buttons;

#define RELEASE_DELAY       0x10
#define PRESS_DELAY         0x15
#define LONG_PRESS_DELAY    0xC7
#define REPEAT_DELAY        0x24

#define BTN_1_PRESS (buttons&0x01)
#define BTN_2_PRESS (buttons&0x02)
#define BTN_3_PRESS (buttons&0x04)
#define BTN_4_PRESS (buttons&0x08)

#define BTN_1_LONG_PRESS (buttons&0x10)
#define BTN_2_LONG_PRESS (buttons&0x20)
#define BTN_3_LONG_PRESS (buttons&0x40)
#define BTN_4_LONG_PRESS (buttons&0x80)

#define BTN_1_RELEASE() (buttons &= ~0x11)
#define BTN_2_RELEASE() (buttons &= ~0x22)
#define BTN_3_RELEASE() (buttons &= ~0x44)
#define BTN_4_RELEASE() (buttons &= ~0x88)

#define BTN_1_REPEAT() (bn[0] = (bn[0] >= LONG_PRESS_DELAY)?(LONG_PRESS_DELAY-REPEAT_DELAY):bn[0])
#define BTN_2_REPEAT() (bn[1] = (bn[1] >= LONG_PRESS_DELAY)?(LONG_PRESS_DELAY-REPEAT_DELAY):bn[1])
#define BTN_3_REPEAT() (bn[2] = (bn[2] >= LONG_PRESS_DELAY)?(LONG_PRESS_DELAY-REPEAT_DELAY):bn[2])
#define BTN_4_REPEAT() (bn[3] = (bn[3] >= LONG_PRESS_DELAY)?(LONG_PRESS_DELAY-REPEAT_DELAY):bn[3])

void disp_init();
void disp_digit(uchar p, uchar n);
void disp_set_brightness(uchar c);

#endif /* _LED_DISPLAY_H_ */
