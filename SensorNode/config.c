#ifndef __AVR__

#include "config.h"

#else /* __AVR__ */

#include <inttypes.h>
#include <avr/eeprom.h>

#include "config.h"

#define EE_VAR(type, name) static volatile type SRAM_##name ; type EEMEM EEPROM_##name
#define EE_READ(var) eeprom_read_block((void*)&SRAM_##var, (const void*)&EEPROM_##var, sizeof(SRAM_##var))
#define EE_WRITE(var)
#define EE_VAL(var) SRAM_##var

EE_VAR(uchar, node_id) = 0;

void cfg_init()
{
    EE_READ(node_id);
}

uchar cfg_node_id()
{
    return EE_VAL(node_id);
}

#endif /* __AVR__ */
