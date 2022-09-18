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

static const uint8_t pkt_len[] = {0, 2, 2, 2, 2, 2, 2, 2, 2};

int main(int argc, char* argv[])
{
	int i, fd = -1;
	char* dev = null;
	int id = -1;
	int cmd = -1;
	int addr = 0;
	int len = 0;
	uint8_t data[MAX_DATA_LEN];
	port_t* port = null;
	
	if(argc > 3)
	{
		dev = argv[1];
		id = strtol(argv[2], null, 0);
		for(i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i)
		{
			if(strcmp(commands[i], argv[3]) == 0) { cmd = i; break; }
		}
		if(cmd < 0) { printf("Unknown command: %s\n", argv[3]); exit(1); }
	}
	else
	{
		help(argv[0]);
		exit(1);
	}
	
	if(argc > 4)
	{
		addr = strtol(argv[4], null, 0);
		if(argc > 5 && cmd == CMD_485_DATA_READ)
		{
			int a = strtol(argv[5], null, 0);
			data[0] = (uint8_t)(a&0xFF); // req len
			len = 1;
			
		}
		else if(argc > 5 && cmd == CMD_485_DATA_WRITE)
		{
			int n = 0;
			for(i = 5; i < argc; ++i)
			{
				int a = strtol(argv[i], null, 0);
				data[n++] = (uint8_t)(a&0xFF);
			}
			len = n;
		}
		else if(argc > 5 && cmd == CMD_485_REG_WRITE8)
		{
			int a = strtol(argv[5], null, 0);
			data[0] = (uint8_t)(a&0xFF);
			len = 1;
		}
		else if(argc > 5 && cmd == CMD_485_REG_WRITE16)
		{
			int a = strtol(argv[5], null, 0);
			data[0] = (uint8_t)(a&0xFF);
			data[1] = (uint8_t)((a>>8)&0xFF);
			len = 2;
		}
		else if(argc > 5 && cmd == CMD_485_REG_WRITE32)
		{
			int a = strtol(argv[5], null, 0);
			data[0] = (uint8_t)(a&0xFF);
			data[1] = (uint8_t)((a>>8)&0xFF);
			data[2] = (uint8_t)((a>>16)&0xFF);
			data[3] = (uint8_t)((a>>24)&0xFF);
			len = 4;
		}
	}
	
	if(dev)
	{
		fd = open_comm(dev);
		if(fd < 0) { printf("Unknown port: %s\n", dev); exit(2); }
		printf("port: %s\n", dev);
		port = proto_init(fd);
	}
	
	if(id >= 0 && cmd >= 0)
	{
		uint8_t plen = pkt_len[cmd] + len;
		uint8_t p[sizeof(packet_t)+plen+2] = {
			SOP_485_CODE0, SOP_485_CODE1, SOP_485_CODE2, SOP_485_CODE3,
			(uint8_t)(id&0xFF), 1 /*from*/, 1, (uint8_t)(cmd&0xFF), plen, (uint8_t)(addr&0xFF), (uint8_t)((addr>>8)&0xFF) };
		packet_t* pkt = (packet_t*)p;
		long rx_timeout = 0;
		
		if(len > 0) for(i = 0; i < len; ++i) pkt->data[i+2] = data[i];
		
		printf("0x%02x: %s 0x%04x\n", id, commands[cmd], addr);
		i = proto_tx(port, pkt);
		rx_timeout = proto_get_time() + 3000; // 3s response timeout
		
		while(i > 0)
		{
			pkt = proto_poll(port);
			if(pkt)
			{
				pkt_dump(stdout, "RX", pkt);
				break;
			}
			if(rx_timeout < proto_get_time())
			{
				printf("Timeout...\n");
				break;
			}
			usleep(1000); // 1ms sleep
		}
	}

	if(port) proto_close(port);
	if(fd >= 0) close(fd);
	return 0;
}
