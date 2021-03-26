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

int main(int argc, char* argv[])
{
	int i, fd = -1;
	char* dev = null;
	int id = -1;
	int cmd = -1;
	int addr = -1;
	
	if(argc > 4)
	{
		dev = argv[1];
		id = strtol(argv[2], null, 0);
		addr = strtol(argv[4], null, 0);
		for(i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i)
		{
			if(strcmp(commands[i], argv[3]) == 0) { cmd = i; break; }
		}
		if(cmd < 0) { printf("Unknown command: %s\n", argv[3]); exit(1); }
	}
	
	if(dev)
	{
		fd = open_comm(dev);
		if(fd < 0) { printf("Unknown port: %s\n", dev); exit(2); }
		printf("port: %s\n", dev);
	}
	
	if(id >= 0 && cmd >= 0 && addr >= 0) printf("0x%02x: %s 0x%04x\n", id, commands[cmd], addr);

	if(fd >= 0) close(fd);
	return 0;
}
