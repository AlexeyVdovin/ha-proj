#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "defines.h"

void cfg_init();

uchar cfg_node_id();
void cfg_set_node_id(uchar val);

ulong cfg_ds1820_period();
void cfg_set_ds1820_period(ulong val);

#endif /* _CONFIG_H_ */
