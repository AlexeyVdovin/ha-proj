#include <stdio.h>

#include "timer.h"
#include "packet.h"

// #include <sys/types.h>
#include <unistd.h>

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
    adc_init();
    
    uchar data[] = { DATA_ID1, DATA_ID2, 0x00, 0x0C, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 };
    pid_t pid = getpid();
    data[3] = (uchar)(pid&0xFF);
    
    do
    {
        packet_t* pkt = rx_packet();
        if(pkt)
        {
            pkt_dump(pkt);
        }
        delay_t(1);
        if(++n > 200)
        {
            n = 0;
            tx_packet((packet_t*)data);
        }
            
    } while(1);
    
	return 0;
}
