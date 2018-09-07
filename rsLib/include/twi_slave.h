#ifndef _TWI_SLAVE_H_
#define _TWI_SLAVE_H_

#include "defines.h"


void twi_init(uint8_t address);
void* get_reg(uint8_t adr);


#endif /* _TWI_SLAVE_H_ */
