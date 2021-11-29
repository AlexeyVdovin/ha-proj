#ifndef _BOILER_H_
#define _BOILER_H_

#define OW_MAX_DEVCES 8  // Max 1W devices per channel

void init_boiler();
void close_boiler();

void setup_boiler_poll();
void handle_boiler();

#endif