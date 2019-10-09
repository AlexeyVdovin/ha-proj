#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "defines.h"

#define MODE_BOOTLOADER     0

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

static void cfg_flush() { eeprom_busy_wait(); }

EE_DEF(long, _ee_zero_);
EE_DEF(uchar, boot_mode);
EE_DEF(uchar, node_id);
EE_DEF(ushort, flash_size);
EE_DEF(ushort, flash_crc);
EE_DEF(long, ds1820_period);

EE_FN_GET(uchar, boot_mode)
EE_FN_SET(uchar, boot_mode)

EE_FN_GET(uchar, node_id)
EE_FN_SET(uchar, node_id)

EE_FN_GET(ushort, flash_size)
EE_FN_SET(ushort, flash_size)

EE_FN_GET(ushort, flash_crc)
EE_FN_SET(ushort, flash_crc)

EE_FN_GET(long, ds1820_period)
EE_FN_SET(long, ds1820_period)

#endif

#endif /* _CONFIG_H_ */
