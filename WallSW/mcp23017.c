#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>
#include <unistd.h>

#include "config.h"
#include "uplink.h"
#include "mcp23017.h"
#include "wiringPi/wiringPi.h"

typedef struct
{
    int dev;
    int n_fd;
} mcp23017_t;

mcp23017_t mcp;

int mcp23017_open(uint8_t bus, uint8_t adr)
{
    char devname[] = "/dev/i2c-0";
    int res, dev = -1;

    do
    {
        if(bus > 9) break;

        devname[sizeof(devname)-2] += bus;
        dev = open(devname, O_RDWR);

        if(dev < 0)
        {
            printf("Error %d: Can't open i2c device %s\n", errno, devname);
            break;
        }

        res = ioctl(dev, I2C_SLAVE, adr);

        if(res < 0)
        {
            close(dev);
            dev = -1;
            printf("Error %d: Can't assign slave i2c address.\n", errno);
            break;
        }

    } while(0);

    return dev;
}

static inline int i2c_io(int dev, char read_write, uint8_t command, int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(dev, I2C_SMBUS, &args);
}

int mcp23017_set_conf(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IOCONA, I2C_SMBUS_BYTE_DATA, &data);
}

int mcp23017_set_dir(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IODIRA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_set_defval(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_DEFVALA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_set_cmpval(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_INTCONA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_set_inten(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_GPINTENA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_get_conf(int dev, uint8_t* conf)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xC3;
        res = i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IOCONA, I2C_SMBUS_BYTE, NULL);
        if(res < 0) break;
        res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *conf = 0x0FF & data.byte;
    } while(0);
    return res;
}

int mcp23017_get_pins(int dev, uint16_t* val)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xC3;
        res = i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IODIRA, I2C_SMBUS_BYTE, NULL);
        if(res < 0) break;
        res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_WORD_DATA, &data);
        if(res < 0) break;
        *val = 0x0FFFF & data.word;
    } while(0);
    return res;
}

// mcp23017 Interrupt pin
static void gpioInterrupt0(void) 
{
    DBG("Interrupt: mcp23017");

}

void handle_mcp23017()
{
    uint8_t c;
    int revents = poll_fds.fds[mcp.n_fd].revents;
    
    if(revents)
    {
        read(getInterruptFS(PIN_INT), &c, 1);
        poll_fds.fds[mcp.n_fd].revents = 0;
        gpioInterrupt0();
    }
}

void init_mcp23017()
{
    int v;
    int dev;
    int res;
    
    mcp.dev  = -1;
    mcp.n_fd = -1;

    dev = mcp23017_open(0, 0x20 + cfg.dev_id);

    if(dev < 0)
    {
        DBG("Error: mcp23017_open() failed %d!", errno);
        return;
    }
    mcp.dev = dev;

    // The INT pins are internally connected
    // INT output pin - Active-low
    res = mcp23017_set_conf(dev, 0x40);

    // All inputs
    res = mcp23017_set_dir(dev, 0xffff);

    // Default value 0x0000
    res = mcp23017_set_defval(dev, 0x0000);

    // Compare inputs with it's previous values
    res = mcp23017_set_cmpval(dev, 0x0000);
    
    pinMode (PIN_INT, INPUT);
    // Read interrupt lane and check it has correct logic level
    v = digitalRead(PIN_INT);
    if(v != 1)
    {
        DBG("Error: Interrupt lane has incorrect logic level: 0");
        return;
    }
    wiringPiISR(PIN_INT, INT_EDGE_FALLING, &gpioInterrupt0);

    // All interrupts enabled
    res = mcp23017_set_inten(dev, 0xffff);
}

void close_mcp23017()
{
    if(mcp.dev > 0) close(mcp.dev);
    mcp.dev = -1;
}

void setup_mcp23017_poll()
{
    int n = poll_fds.n;
    mcp.n_fd = n;
    poll_fds.n += 2;
    
    poll_fds.fds[n].fd = getInterruptFS(PIN_INT);
    poll_fds.fds[n].events = POLLPRI;
    poll_fds.fds[n].revents = 0;
    ++n;
    poll_fds.fds[n].fd = getInterruptFS(PIN_PA7);
    poll_fds.fds[n].events = POLLPRI;
    poll_fds.fds[n].revents = 0;
}
