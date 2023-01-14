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
#include "pca9554.h"


typedef struct
{
    int      dev;
    uint8_t  out;
    
} pca9554_t;

pca9554_t pca;

int pca9554_open(uint8_t bus, uint8_t adr)
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

int pca9554_set_dir(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, PCA9554_DIR, I2C_SMBUS_BYTE_DATA, &data);
}

int pca9554_set_inv(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, PCA9554_INV, I2C_SMBUS_BYTE_DATA, &data);
}

int pca9554_set_pins(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, PCA9554_OUTPUT, I2C_SMBUS_BYTE_DATA, &data);
}

int pca9554_get_pins(int dev, uint8_t* val)
{
    int res = -1;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, PCA9554_INPUT, I2C_SMBUS_BYTE_DATA, &data);
    if(res >= 0) *val = 0x0FF & data.byte;
    return res;
}

void init_pca9554()
{
    int i, v;
    int dev;
    int res;
    uint16_t p;
    
    pca.dev  = -1;
    pca.out  = 0;
    
    dev = pca9554_open(0, PCA9554_SLAVE_ADDR);

    if(dev < 0)
    {
        DBG("Error: pca9554_open() failed %d!", errno);
        return;
    }
    pca.dev = dev;

    res = pca9554_set_inv(dev, 0x00);
    res = pca9554_set_pins(dev, 0x00);

    // All outputs
    res = pca9554_set_dir(dev, 0x00);
}

void close_pca9554()
{
    if(pca.dev > 0) close(pca.dev);
    pca.dev = -1;
}

int get_pca9554(int n)
{
    int val;

    if(n < 0 || n > 7 || pca.dev < 0)
    {
        DBG("Error: Invalid parameter: %d", n);
        return -1;
    }

    val = (pca.out & (1 << n)) ? 1 : 0;
    return val;
}

void set_pca9554(int n, int val)
{
    int res;

    if(n < 0 || n > 7 || pca.dev < 0)
    {
        DBG("Error: Invalid parameter: %d, %d", n, val);
        return;
    }
    
    pca.out &= ~(1 << n);
    pca.out |= val ? (1 << n) : 0;
    
    res = pca9554_set_pins(pca.dev, pca.out);
    // DBG("pca9554_set_pins(0x%02x)", pca.out);
}
