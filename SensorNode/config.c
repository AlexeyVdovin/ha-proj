#ifndef __AVR__

#include "config.h"

#else /* __AVR__ */

#include <inttypes.h>
#include <avr/eeprom.h>

#include "config.h"

#define EE_VAR(type, name) static volatile type SRAM_##name ; type EEMEM EEPROM_##name
#define EE_READ(var) eeprom_read_block((void*)&SRAM_##var, (const void*)&EEPROM_##var, sizeof(SRAM_##var))
#define EE_WRITE(var) eeprom_update_block((void*)&SRAM_##var, (void*)&EEPROM_##var, sizeof(SRAM_##var))
#define EE_VAL(var) SRAM_##var

#define EE_FN_GET(type, var) type cfg_##var() { return EE_VAL(var); }
#define EE_FN_SET(type, var) void cfg_set__##var(type val) { EE_VAL(var) = val; eeprom_busy_wait(); EE_WRITE(var); }

EE_VAR(uchar, node_id) = 0x0A;
EE_VAR(ushort, ds1820_period) = 1000; /* 10 sec */

void cfg_init()
{
    eeprom_busy_wait();
    
    EE_READ(node_id);
    EE_READ(ds1820_period);
}

EE_FN_GET(uchar, node_id)
EE_FN_SET(uchar, node_id)

EE_FN_GET(ushort, ds1820_period)
EE_FN_SET(ushort, ds1820_period)

#endif /* __AVR__ */
