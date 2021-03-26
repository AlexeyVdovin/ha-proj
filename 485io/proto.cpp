#include <stdlib.h>
#include <unistd.h>

#include "proto.h"

#define null NULL

/* Calculating XMODEM CRC-16 in 'C'
   ================================
   Reference model for the translated code */

#define poly 0x1021

/* On entry, addr=>start of data
             len = length of data
             crc = incoming CRC     */
static uint16_t crc16(uint8_t* addr, int len, uint32_t crc)
{
    int i;
    for(; len > 0; len--)                     /* Step through bytes in memory */
    {
        crc = crc ^ (*addr++ << 8);           /* Fetch byte from memory, XOR into CRC top byte*/
        for (i=0; i<8; i++)                   /* Prepare to rotate 8 bits */
        {
            crc = crc << 1;                   /* rotate */
            if (crc & 0x10000)                /* bit 15 was set (now bit 16)... */
                crc = (crc ^ poly) & 0x0FFFF; /* XOR with XMODEM polynomic */
                                              /* and ensure CRC remains 16-bit value */
        }                                     /* Loop for 8 bits */
    }                                         /* Loop until num=0 */
    return (uint16_t)crc;                     /* Return updated CRC */
}

t_port* proto_init(int fd)
{
	t_port* port = (t_port*)malloc(sizeof(t_port));
	port->fd = fd;
	return port;
}

void proto_close(t_port* port)
{
	free(port);
}

int proto_tx(t_port* port, t_packet* packet)
{
    int rc, len = packet->len + sizeof(t_packet);
	uint8_t* p = (uint8_t*)packet;
	uint16_t crc = crc16(p, len, 0);
	uint8_t* c = (uint8_t*)&crc;
	packet->data[packet->len + 0] = c[0];
	packet->data[packet->len + 1] = c[1];
    rc = write(port->fd, p, len+2);
	return rc;
}

t_packet* proto_poll(t_port* port)
{

	return null;
}


/*
static uchar rx_pkt[sizeof(packet_t)+MAX_DATA_LEN+2];
static uchar _rx_pos = 0;
static long rx_time = 0;

static packet_t* rx_packet()
{
    packet_t *pkt = (packet_t*)rx_pkt;
    register uchar rx_pos = _rx_pos;

    // Check for timeout    
    if(rx_pos && rx_time && get_time() > rx_time) rx_pos = 0;

    while(rx_count())
    {
        int c = rx_byte();
        if(c < 0) break;
        register uchar u = (uchar)(c & 0x00FF);
        
        if(rx_pos == 0 || (rx_pos == 1 && u == DATA_ID1))
        {
            rx_pos = 0;
            if(u != DATA_ID1) continue;
            else rx_time = get_time()+PACKET_RX_TIMEOUT;
        }
        if(rx_pos == 1 && u != DATA_ID2) { rx_pos = 0; continue; }
        if(rx_pos == sizeof(packet_t)-1 && pkt->len > MAX_DATA_LEN) { rx_pos = 0; continue; }
        if(rx_pos < sizeof(packet_t) || rx_pos < sizeof(packet_t) + pkt->len + 2)
        { 
            rx_pkt[rx_pos++] = u;
        }
        if(rx_pos >= sizeof(packet_t) + pkt->len + 2)
        {
            rx_pos = 0;
            ushort* crc = (ushort*)(pkt->data + pkt->len);
            if(pkt->len > 0 && *crc == packet_crc(pkt)) { _rx_pos = rx_pos; return pkt; }
        }
    }
    _rx_pos = rx_pos;
    return NULL;
}

static void tx_packet(packet_t* pkt)
{
    uchar i = pkt->len + sizeof(packet_t)+2, *c = (uchar*)pkt;
    ushort* crc = (ushort*)(pkt->data + pkt->len);
    *crc = packet_crc(pkt);
    while(--i) tx_byte(*c++);
}
*/