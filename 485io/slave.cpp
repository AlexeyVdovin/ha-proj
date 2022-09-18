#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define null NULL

#define AFTERX(x) B ## x
#define XTTY_BAUD(x) AFTERX(x)
#define TTY_BAUD XTTY_BAUD(BAUD)


#include "proto.h"
/*
	CMD_485_PING = 0,
	CMD_485_REG_READ8,
	CMD_485_REG_READ16,
	CMD_485_REG_READ32,
	CMD_485_DATA_READ,
	CMD_485_REG_WRITE8,
	CMD_485_REG_WRITE16,
	CMD_485_REG_WRITE32,
	CMD_485_DATA_WRITE
*/

const char* commands[] =
{
	"ping",
	"read8",
	"read16",
	"read32",
	"read",
	"write8",
	"write16",
	"write32",
	"write"
};

void help(char* cmd)
{
	printf("Help ...\n");
}

int open_comm(char* dev)
{
	int flags, fd = -1;
    struct termios tty;
	
	do
	{
		fd = open(dev, O_RDWR | O_NDELAY | O_NOCTTY | O_NONBLOCK);
		if(fd < 0) break;

		if(tcgetattr(fd, &tty) < 0)
		{
			printf("tcgetattr() failed. Error: %d\n", errno);
			close(fd);
			fd = -1;
			break;
		}

		// Reset terminal to RAW mode
		cfmakeraw(&tty);

		// Set baud rate
		cfsetspeed(&tty, TTY_BAUD);

		if(tcsetattr(fd, TCSAFLUSH, &tty) < 0)
		{
			printf("tcsetattr() failed. Error: %d\n", errno);
			close(fd);
			fd = -1;
			break;
		}
		flags = fcntl(fd, F_GETFL, 0);
		if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		{
			printf("fcntl(F_SETFL) failed. Error: %d\n", errno);
			close(fd);
			fd = -1;
			break;
		}
	} while(0);

	return fd;
}

static const uint8_t pkt_len[] = {0, 2, 2, 2, 3, 2, 2, 2, 3};
static uint32_t regs[0x100];

void process_pkt(port_t* port, packet_t* pkt, int id)
{
	uint8_t tx[sizeof(packet_t)+MAX_DATA_LEN+2] = {
			SOP_485_CODE0, SOP_485_CODE1, SOP_485_CODE2, SOP_485_CODE3,
			pkt->src, (uint8_t)(id&0xFF)/*from*/, 1, 0, 0 };
	packet_t* p = null;
	uint8_t i, len = 0;
	uint16_t addr = 0;
	
	// printf("process_pkt()\n");
	
	switch(pkt->cmd)
	{
		case CMD_485_PING:
			p = (packet_t*)tx;
			p->len = 0;
			p->cmd = 0x80 | CMD_485_PING;
			break;
		case CMD_485_REG_READ8:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			if(addr < sizeof(regs))
			{
				p = (packet_t*)tx;
				p->len = 1;
				p->cmd = 0x80 | CMD_485_REG_READ8;
				p->data[0] = regs[addr];
			}
			break;
		}
		case CMD_485_REG_READ16:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			if(addr+1 < sizeof(regs))
			{
				p = (packet_t*)tx;
				p->len = 2;
				p->cmd = 0x80 | CMD_485_REG_READ16;
				p->data[0] = regs[addr];
				p->data[1] = regs[addr+1];
			}
			break;
		}
		case CMD_485_REG_READ32:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			if(addr+3 < sizeof(regs))
			{
				p = (packet_t*)tx;
				p->len = 4;
				p->cmd = 0x80 | CMD_485_REG_READ16;
				p->data[0] = regs[addr];
				p->data[1] = regs[addr+1];
				p->data[2] = regs[addr+2];
				p->data[3] = regs[addr+3];
			}
			break;
		}
		case CMD_485_DATA_READ:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			len = pkt->data[2];
			if(len && addr+len-1 < sizeof(regs))
			{
				p = (packet_t*)tx;
				p->len = len;
				p->cmd = 0x80 | CMD_485_DATA_READ;
				for(i = 0; i < len; ++i) p->data[i] = regs[addr+i];
			}
			break;
		}
		case CMD_485_REG_WRITE8:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			if(addr < sizeof(regs))
			{
				regs[addr] = pkt->data[2];
				p = (packet_t*)tx;
				p->len = 0;
				p->cmd = 0x80 | CMD_485_REG_WRITE8;
			}
			break;
		}
		case CMD_485_REG_WRITE16:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			if(addr+1 < sizeof(regs))
			{
				regs[addr] = pkt->data[2];
				regs[addr+1] = pkt->data[3];
				p = (packet_t*)tx;
				p->len = 0;
				p->cmd = 0x80 | CMD_485_REG_WRITE16;
			}
			break;
		}
		case CMD_485_REG_WRITE32:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			if(addr+3 < sizeof(regs))
			{
				regs[addr] = pkt->data[2];
				regs[addr+1] = pkt->data[3];
				regs[addr+2] = pkt->data[4];
				regs[addr+3] = pkt->data[5];
				p = (packet_t*)tx;
				p->len = 0;
				p->cmd = 0x80 | CMD_485_REG_WRITE32;
			}
			break;
		}
		case CMD_485_DATA_WRITE:
		{
			addr = pkt->data[0] | (pkt->data[1]<<8);
			len = pkt->data[2];
			if(len && addr+len-1 < sizeof(regs))
			{
				for(i=0; i < len; ++i) regs[addr+i] = pkt->data[3+i];
				p = (packet_t*)tx;
				p->len = 0;
				p->cmd = 0x80 | CMD_485_DATA_WRITE;
			}
			break;
		}
			
		default:
			break;
	}
	if(p) proto_tx(port, p);
};


int main(int argc, char* argv[])
{
	int i, fd = -1;
	char* dev = null;
	int id = -1;
	int cmd = -1;
	int addr = -1;
	int len = -1;
	uint8_t data[MAX_DATA_LEN];
	port_t* port = null;
	
	if(argc > 2)
	{
		dev = argv[1];
		id = strtol(argv[2], null, 0);
	}
	else
	{
		help(argv[0]);
		exit(1);
	}
	
	if(dev)
	{
		fd = open_comm(dev);
		if(fd < 0) { printf("Unknown port: %s\n", dev); exit(2); }
		printf("port: %s\n", dev);
		port = proto_init(fd);
	}
	
	if(id >= 0)
	{
		packet_t* pkt = null;
		
		while(1)
		{
			pkt = proto_poll(port);
			if(pkt)
			{
				pkt_dump(stdout, "RX", pkt);
				process_pkt(port, pkt, id);
			}
			usleep(1000); // 1ms sleep
		}
	}

	if(port) proto_close(port);
	if(fd >= 0) close(fd);
	return 0;
}
