#ifndef __AVR__
#include <stdio.h>

#else  /* __AVR__ */

#endif /* __AVR__ */

#include "timer.h"
#include "sio.h"
#include "packet.h"
#include "rs485.h"

#define PACKET_RX_TIMEOUT ((1000000/(BAUD/10)*(sizeof(packet_t)+MAX_DATA_LEN+2)*2+5000)/10000)

#define rx_count()   sio_rxcount()
#define rx_byte()    sio_getchar()
#define tx_byte(c)   sio_putchar(c)

#include "packetio.h"

packet_t* rs485_rx_packet()
{
    return rx_packet();
}

void rs485_tx_packet(packet_t* pkt)
{
    tx_packet(pkt);
}

