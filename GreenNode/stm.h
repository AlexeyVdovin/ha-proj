#ifndef _STM_H_
#define _STM_H_

#define STM_SLAVE_ADDR  0x30

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