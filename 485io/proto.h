#ifndef _PROTO_H_
#define _PROTO_H_
#include <stdio.h>
#include <stdint.h>

#define BAUD 9600
#define MAX_DATA_LEN  110
#define PACKET_RX_TIMEOUT  20

enum
{
	SOP_485_CODE0 = 0x21,
	SOP_485_CODE1 = 0x34,
	SOP_485_CODE2 = 0x38,
	SOP_485_CODE3 = 0x35
};

enum
{
	CMD_485_PING = 0,
	CMD_485_REG_READ8,
	CMD_485_REG_READ16,
	CMD_485_REG_READ32,
	CMD_485_DATA_READ,
	CMD_485_REG_WRITE8,
	CMD_485_REG_WRITE16,
	CMD_485_REG_WRITE32,
	CMD_485_DATA_WRITE
};

typedef struct
{
	uint8_t  sop[4];  // Start of packet
	uint8_t  dst; 
	uint8_t  src;
	uint8_t  trid; // Transaction id
	uint8_t  cmd;  // Command/Reply code
	uint8_t  len;  // Data length
	uint8_t  data[];
} packet_t;

typedef struct
{
	int      fd;
	uint8_t  rx_pkt[sizeof(packet_t)+MAX_DATA_LEN+2]; // +2 for CRC
	uint8_t  rx_pos;
	long     rx_time;
	
} port_t;

port_t* proto_init(int fd);
void proto_close(port_t* port);

packet_t* proto_poll(port_t* port);
int proto_tx(port_t* port, packet_t* packet);
long proto_get_time();

void pkt_dump(FILE* f, const char* s, packet_t* packet);

#endif /* _PROTO_H_ */
