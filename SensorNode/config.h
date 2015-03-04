#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "defines.h"

void cfg_init();

#ifdef __AVR__
#include <inttypes.h>
#include <avr/eeprom.h>

#define EE_DEF(type, name) extern type SRAM_##name ; extern type EEMEM EEPROM_##name
#define EE_VAR(type, name) type SRAM_##name ; type EEMEM EEPROM_##name
#define EE_READ(var) eeprom_read_block((void*)&SRAM_##var, (const void*)&EEPROM_##var, sizeof(SRAM_##var))
#define EE_WRITE(var) eeprom_update_block((void*)&SRAM_##var, (void*)&EEPROM_##var, sizeof(SRAM_##var))
#define EE_VAL(var) SRAM_##var

#define EE_FN_GET(type, var) static type cfg_##var() { return EE_VAL(var); }
#define EE_FN_SET(type, var) static void cfg_set_##var(type val) { EE_VAL(var) = val; eeprom_busy_wait(); EE_WRITE(var); }

EE_DEF(uchar, node_id);
EE_DEF(ulong, node_mac);
EE_DEF(ushort, flash_size);
EE_DEF(ushort, flash_crc);

EE_FN_GET(uchar, node_id)
EE_FN_SET(uchar, node_id)

EE_FN_GET(ulong, node_mac)

EE_FN_GET(ushort, flash_size)
EE_FN_SET(ushort, flash_size)

EE_FN_GET(ushort, flash_crc)
EE_FN_SET(ushort, flash_crc)

#endif

/*
uchar cfg_node_id();
void cfg_set_node_id(uchar val);

ulong cfg_node_mac();

ushort cfg_flash_size();
void cfg_set_flash_size(ushort val);

ushort cfg_flash_crc();
void cfg_set_flash_crc(ushort val);

ushort cfg_ds1820_period();
void cfg_set_ds1820_period(ushort val);
*/

#endif /* _CONFIG_H_ */
