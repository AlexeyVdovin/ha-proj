#ifndef __AVR__

#include "config.h"

#else /* __AVR__ */

#include "config.h"

EE_VAR(uchar, boot_mode) = 0; // boot to bootloader
EE_VAR(uchar, node_id) = 0x0A;
EE_VAR(ulong, node_mac) = _CFG_NODE_MAC; // Defined at compile time
EE_VAR(ushort, flash_size) = 0;
EE_VAR(ushort, flash_crc) = 0;
EE_VAR(ulong, ds1820_period) = 1000; /* 10 sec */

void cfg_init()
{
    eeprom_busy_wait();
    
    EE_READ(boot_mode);
    EE_READ(node_id);
    EE_READ(node_mac);
    EE_READ(flash_size);
    EE_READ(flash_crc);
}

#endif /* __AVR__ */
