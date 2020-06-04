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

#include "mcp23017.h"

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

int mcp23017_set_conf(int dev, uint8_t conf)
{
    union i2c_smbus_data data;
    data.byte = conf;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IOCONA, I2C_SMBUS_BYTE_DATA, &data);
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

int mcp23017_get_dirAB(int dev, uint16_t* dir)
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
        *dir = 0x0FFFF & data.word;
    } while(0);
    return res;
}

#if 0
int main(int argc, char* argv[])
{
    uint8_t sa = 0x20;
    int dev;
    int res;

    if(argc > 1)
    {
        sa = (uint8_t)strtol(argv[1], NULL, 0);
    }

    dev = mcp23017_open(0, sa);

    if(dev < 0)
    {
        printf("Error: mcp23017_open() failed %d!\n", errno);
        exit(1);
    }
    
    res = mcp23017_set_conf(dev, 0x02);
    
    uint8_t conf = 0xcc;
    res = mcp23017_get_conf(dev, &conf);
    printf("(%d) Conf = 0x%02X\n", res, conf);

    uint16_t dir = 0xccee;
    res = mcp23017_get_dirAB(dev, &dir);
    printf("(%d) Direction = 0x%04X\n", res, dir);

    close(dev);

    return 0;
}
#endif