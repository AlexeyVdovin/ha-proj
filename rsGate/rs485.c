#include <stdio.h>

#include "timer.h"
#include "sio.h"
#include "rs485.h"


static uchar rx_pkt[sizeof(packet_t)+MAX_DATA_LEN+2] = { 0 };
static rx_pos = 0;

void rs485_init()
{
    sio_init();
}

packet_t* rs485_rx_packet()
{
    packet_t *pkt = (packet_t*)rx_pkt;
    
    while(sio_rxcount())
    {
        int c = sio_getchar();
        if(c < 0) break;
        uchar u = (uchar)(c & 0x00FF);
        if(rx_pos == 0 && u != DATA_ID1) continue;
        if(rx_pos == 1 && u != DATA_ID2) continue;
        if(rx_pos > 5 && rx_pos < sizeof(packet_t) + pkt->len + 2)
        { 
            rx_pkt[rx_pos] = u;
        }
        if(rx_pos == sizeof(packet_t) + pkt->len + 2)
        {
            rx_pos = 0;
            ushort* crc = (ushort*)(pkt->data + pkt->len);
            if(pkt->len > 0 && *crc == packet_crc(pkt)) return pkt;
            printf("rs485: CRC missmatch!\n");
        }
    }
    return NULL;
}

void rs485_tx_packet(packet_t* pkt)
{
    uchar i, *c = (uchar*)pkt;
    ushort* crc = (ushort*)(pkt->data + pkt->len);
    *crc = packet_crc(pkt);
    for(i = 0; i < pkt->len + sizeof(packet_t)+2; ++i) sio_putchar(c[i]);
}

