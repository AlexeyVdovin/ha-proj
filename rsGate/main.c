#include <stdio.h>
#include <unistd.h>

#include "timer.h"
#include "rs485.h"
#include "packet.h"

static void pkt_dump(packet_t* pkt)
{
    int i;
    printf("Pkt: %02X%02X %02X -> %02X  (%02X) [%d]:", pkt->id[0], pkt->id[1], pkt->from, pkt->to, pkt->flags, pkt->len);
    for(i = 0; i < pkt->len; ++i) { printf(" %02X", pkt->data[i]); }
    printf(" %02X%02X\n", pkt->data[pkt->len], pkt->data[pkt->len+1]);
}

int main(int argc, char** argv)
{
    int n = 0;
    timer_init();
    packet_init();
    rs485_init();
    
    // TODO: Add request/response guard mechanism  (enqueue subsequent requests)
    do
    {
        packet_t* pkt = rs485_rx_packet();
        if(pkt)
        {
            printf("rs485 -> tcp ");
            pkt_dump(pkt);
            tx_packet(pkt);
        }
        pkt = rx_packet();
        if(pkt)
        {
            printf("tcp -> rs485 ");
            pkt_dump(pkt);
            rs485_tx_packet(pkt);
        }
        usleep(1000000/(BAUD/10)*2); // Delay for ~2 chars 
    } while(1);
    
	return 0;
}
