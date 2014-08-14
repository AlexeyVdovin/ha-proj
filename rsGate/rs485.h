#ifndef _RS485_H_
#define _RS485_H_

#include "defines.h"
#include "packet.h"

void rs485_init();

packet_t* rs485_rx_packet();
void rs485_tx_packet(packet_t* pkt);



#endif /* _RS485_H_ */
