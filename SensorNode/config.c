#ifndef __AVR__

#include "config.h"

#else /* __AVR__ */

#include "config.h"

EE_VAR(uchar, node_id) = 0x0A;
EE_VAR(ulong, node_mac) = _CFG_NODE_MAC; // Defined at compile time
EE_VAR(ushort, flash_size) = 0;
EE_VAR(ushort, flash_crc) = 0;

void cfg_init()
{
    eeprom_busy_wait();
    
    EE_READ(node_id);
    EE_READ(node_mac);
    EE_READ(flash_size);
    EE_READ(flash_crc);
}

#endif /* __AVR__ */
