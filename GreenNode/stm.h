#ifndef _STM_H_
#define _STM_H_

#define STM_SLAVE_ADDR  0x30

#define STM_ENABLE_PM 0x8000
#define STM_LOW_PWR   0x0001
#define STM_CTL_DR1   0x0002
#define STM_CTL_DR2  	0x0004

#define STM_HEARTBEAT 0x00
#define STM_STATUS    0x08
#define STM_CONTROL   0x0A
#define STM_DC_12V    0x10
#define STM_DC_BATT   0x12
#define STM_DC_5V0    0x14
#define STM_DC_3V3    0x16
#define STM_DC_REF    0x18
#define STM_TEMPR     0x1A

void init_stm();
void close_stm();

void setup_stm_poll();
void handle_stm();

#endif