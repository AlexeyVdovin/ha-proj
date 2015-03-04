#include <stdio.h>
#include <unistd.h>

#include "timer.h"
#include "sio.h"
#include "rs485.h"
#include "udp.h"
#include "packet.h"
#include "queue.h"

#define RESPONSE_TIMEOUT 10 /* 100ms */

#define MyID 0x01

static void pkt_dump(packet_t* pkt)
{
    int i;
    printf("Pkt: %02X%02X %02X -> %02X -> %02X  (%02X) %02X [%d]:", pkt->id[0], pkt->id[1], pkt->from, pkt->via, pkt->to, pkt->flags, pkt->seq, pkt->len);
    for(i = 0; i < pkt->len; ++i) { printf(" %02X", pkt->data[i]); }
    printf(" %02X%02X\n", pkt->data[pkt->len], pkt->data[pkt->len+1]);
}

static void init()
{
    timer_init();
    sio_init();
    udp_init();
}

int main(int argc, char** argv)
{
    int n = 0;
    int busy = 0;
    long t;    
    uchar seq;
    
    init();
    
    do
    {
        packet_t* pkt = rs485_rx_packet();
        if(pkt && pkt->via == MyID) pkt = NULL;
        if(pkt)
        {
            printf("rs485 -> udp ");
            pkt_dump(pkt);
            pkt->via = MyID;
            udp_tx_packet(pkt);
            if(busy && seq == pkt->seq) busy = 0;
        }
        if(busy && (get_time() > t))
        {
            // Response timeout
            busy = 0;
            // TODO: Notify sender ??
        }
        pkt = udp_rx_packet();
        if(pkt && pkt->via == MyID) pkt = NULL;
        if(pkt && busy)
        {
            queue_put(pkt);
            pkt = NULL;
        }
        if(!busy && (pkt || (pkt = queue_get())))
        {
            busy = 1;
            seq = pkt->seq;
            t = get_time() + RESPONSE_TIMEOUT;
            printf("udp -> rs485 ");
            pkt_dump(pkt);
            pkt->via = MyID;
            rs485_tx_packet(pkt);
        }
        usleep(1000000/(BAUD/10)*2); // Delay for ~2 chars 
    } while(1);
    
	return 0;
}

