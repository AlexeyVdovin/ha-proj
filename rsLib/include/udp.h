#ifndef _UDP_H_
#define _UDP_H_

#include "defines.h"
#include "packet.h"

void udp_init();

packet_t* udp_rx_packet();
void udp_tx_packet(packet_t* pkt);

#endif /* _UDP_H_ */
