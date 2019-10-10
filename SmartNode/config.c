#ifndef __AVR__

#include "config.h"

#else /* __AVR__ */

#include "config.h"

EE_VAR(long, _ee_zero_) = 0;
EE_VAR(uchar, boot_mode) = 0; // boot to bootloader
EE_VAR(uchar, node_id) = 0x0A;
EE_VAR(ushort, flash_size) = 0;
EE_VAR(ushort, flash_crc) = 0;
EE_VAR(long, ds1820_period) = 1000; /* 10 sec */

void cfg_init()
{
    eeprom_busy_wait();
    
    EE_READ(boot_mode);
    EE_READ(node_id);
    EE_READ(flash_size);
    EE_READ(flash_crc);
    EE_READ(ds1820_period);
}

#endif /* __AVR__ */
