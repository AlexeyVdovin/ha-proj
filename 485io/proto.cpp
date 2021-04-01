#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

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

long proto_get_time()
{
	long ms;
	struct timeval tv;
	gettimeofday(&tv, null);
	ms = tv.tv_sec * 1000 + tv.tv_usec/1000;
	return ms;
}

port_t* proto_init(int fd)
{
	port_t* port = (port_t*)malloc(sizeof(port_t));
	port->fd = fd;
	port->rx_pos = 0;
	port->rx_time = 0;
	return port;
}

void proto_close(port_t* port)
{
	free(port);
}

void pkt_dump(FILE* f, const char* s, packet_t* p)
{
	int i;
	fprintf(f, "%s: 0x%02X->0x%02X 0x%02X cmd:0x%02X [%d]: ", s, p->src, p->dst, p->trid, p->cmd, p->len);
	for(i = 0; i < p->len; ++i) { fprintf(f, "0x%02X ", p->data[i]); }
	fprintf(f, "0x%04X\n", p->data[p->len] + (p->data[p->len+1]<<8));
	fflush(f);
}

static inline uint16_t packet_crc(packet_t* pkt)
{
    int len = pkt->len + sizeof(packet_t);
	uint8_t* p = (uint8_t*)pkt;
	uint16_t crc = crc16(p, len, 0);
	return crc;
}

int proto_tx(port_t* port, packet_t* packet)
{
    int rc, len = packet->len + sizeof(packet_t);
	uint8_t* p = (uint8_t*)packet;
	uint16_t crc = packet_crc(packet);
	uint8_t* c = (uint8_t*)&crc;
	packet->data[packet->len + 0] = c[0];
	packet->data[packet->len + 1] = c[1];
	pkt_dump(stdout, "TX", packet);
    rc = write(port->fd, p, len+2);
	return rc;
}

packet_t* proto_poll(port_t* port)
{
	int rc;
    packet_t *pkt = (packet_t*)port->rx_pkt;
    uint8_t rx_pos = port->rx_pos;
	uint8_t  c;

    // Check for timeout    
    if(rx_pos && port->rx_time && proto_get_time() > port->rx_time) rx_pos = 0;
	
	do
	{
		rc = read(port->fd, &c, 1);
		if(rc < 0) { pkt = null; break; }
		
		if(rx_pos == 0 && c != SOP_485_CODE0) continue;
		if(rx_pos == 1 && c != SOP_485_CODE1) { rx_pos = 0; continue; }
		if(rx_pos == 2 && c != SOP_485_CODE2) { rx_pos = 0; continue; }
		if(rx_pos == 3 && c != SOP_485_CODE3) { rx_pos = 0; continue; }
		
		if(rx_pos == sizeof(packet_t)-1 && pkt->len > MAX_DATA_LEN) { rx_pos = 0; continue; }
			
		if(rx_pos < sizeof(packet_t) || rx_pos < sizeof(packet_t) + pkt->len + 2)
        { 
            port->rx_pkt[rx_pos++] = c;
			port->rx_time = proto_get_time()+PACKET_RX_TIMEOUT;
		}

		if(rx_pos >= sizeof(packet_t) + pkt->len + 2)
        {
            rx_pos = 0;
            uint16_t crc = (pkt->data[pkt->len+1] << 8) + pkt->data[pkt->len];
            if(pkt->len > 0 && crc == packet_crc(pkt)) break;
        }
	} while(1);
	
	port->rx_pos = rx_pos;

	return pkt;
}

