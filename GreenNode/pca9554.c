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

pca9554_t pca[PCA9554_MAX_DEV];

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

void init_pca9554(int id)
{
    int i, v;
    int dev;
    int res;
    uint16_t p;
    
    if(id < 1 || id >= PCA9554_MAX_DEV)
    {
        DBG("Invalid PCA9554 ID:%d", id);
        return;
    }
    
    pca[id].dev  = -1;
    pca[id].out  = 0;
    
    dev = pca9554_open(0, 0x20 + id);

    if(dev < 0)
    {
        DBG("Error: pca9554_open(%d) failed %d!", id, errno);
        return;
    }
    pca[id].dev = dev;

    res = pca9554_set_inv(dev, 0x00);
    res = pca9554_set_pins(dev, 0x00);

    // All outputs
    res = pca9554_set_dir(dev, 0x00);
}

void close_pca9554(int id)
{
    if(id < 1 || id >= PCA9554_MAX_DEV)
    {
        DBG("Invalid PCA9554 ID:%d", id);
        return;
    }
    if(pca[id].dev > 0) close(pca[id].dev);
    pca[id].dev = -1;
}

int get_pca9554(int id, int n)
{
    int val;

    if(id < 1 || id >= PCA9554_MAX_DEV || n < 0 || n > 7 || pca[id].dev < 0)
    {
        DBG("Error: Invalid parameter: %d, %d", id, n);
        return -1;
    }

    val = (pca[id].out & (1 << n)) ? 1 : 0;
    return val;
}

void set_pca9554(int id, int n, int val)
{
    int res;

    if(id < 1 || id >= PCA9554_MAX_DEV || n < 0 || n > 7 || pca[id].dev < 0)
    {
        DBG("Error: Invalid parameter: %d, %d, %d", id, n, val);
        return;
    }
    
    pca[id].out &= ~(1 << n);
    pca[id].out |= val ? (1 << n) : 0;
    
    res = pca9554_set_pins(pca[id].dev, pca[id].out);
}
