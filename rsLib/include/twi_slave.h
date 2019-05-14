#ifndef _TWI_SLAVE_H_
#define _TWI_SLAVE_H_

#include "defines.h"
/*
REGS :
0x00 - ushort 3.3V Pi Zero
0x02 - ushort 5V Pi Zero
0x04 - ushort 3.3V 1Wire
0x0A - ushort Shutdown timeout (0 - off)
0x0C - ushort 12V Input
0x0E - ushort 12V Power
0x10 - ushort Activity
0x12 - ushort Status
0x14 - ushort LED/Relay control

0x40 - reg count
*/

enum
{
    CTRL_RELAY_1 = 0x01,
    CTRL_RELAY_2 = 0x02,
    CTRL_RELAY_3 = 0x04,
    CTRL_RELAY_4 = 0x08,
    CTRL_LED_RED = 0x100,
    CTRL_LED_GRN = 0x200
};


void twi_init(uint8_t address);

void twi_enable();
uint8_t twi_disable(); // Return: 0 - Success, 1 - Fail

void* get_reg(uint8_t adr);


#endif /* _TWI_SLAVE_H_ */
