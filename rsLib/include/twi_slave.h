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

void twi_init(uint8_t address);
void* get_reg(uint8_t adr);


#endif /* _TWI_SLAVE_H_ */
