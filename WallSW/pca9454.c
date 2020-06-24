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
#include "pca9454.h"

typedef struct
{
    int      dev;
    uint8_t  out;
    
} pca9454_t;

pca9454_t pca[PCA9454_MAX_DEV];

int pca9454_open(uint8_t bus, uint8_t adr)
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

int pca9454_set_dir(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, PCA9454_DIR, I2C_SMBUS_BYTE_DATA, &data);
}

int pca9454_set_inv(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, PCA9454_INV, I2C_SMBUS_BYTE_DATA, &data);
}

int pca9454_set_pins(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, PCA9454_OUTPUT, I2C_SMBUS_BYTE_DATA, &data);
}

int pca9454_get_pins(int dev, uint8_t* val)
{
    int res = -1;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, PCA9454_INPUT, I2C_SMBUS_BYTE_DATA, &data);
    if(res >= 0) *val = 0x0FF & data.byte;
    return res;
}

void init_pca9454(int id)
{
    int v;
    int dev;
    int res;
    uint16_t p;
    
    if(id < 1 || id >= PCA9454_MAX_DEV)
    {
        DBG("Invalid PCA9454 ID:%d", id);
        return;
    }
    
    pca[id].dev  = -1;
    pca[id].out  = 0;
    
    dev = pca9454_open(0, 0x20 + id);

    if(dev < 0)
    {
        DBG("Error: pca9454_open(%d) failed %d!", id, errno);
        return;
    }
    pca[id].dev = dev;

    res = pca9454_set_inv(dev, 0x00);
    res = pca9454_set_pins(dev, 0x00);

    // All outputs
    res = pca9454_set_dir(dev, 0x00);
}

void close_pca9454(int id)
{
    if(id < 1 || id >= PCA9454_MAX_DEV)
    {
        DBG("Invalid PCA9454 ID:%d", id);
        return;
    }
    if(pca[id].dev > 0) close(pca[id].dev);
    pca[id].dev = -1;
}

void set_pca9454(int id, int n, int val)
{
    int res;

    if(id < 1 || id >= PCA9454_MAX_DEV || n < 0 || n > 7 || pca[id].dev < 0)
    {
        DBG("Error: Invalid parameter: %d, %d, %d", id, n, val);
        return;
    }
    
    pca[id].out &= ~(1 << n);
    pca[id].out |= val ? (1 << n) : 0;
    
    res = pca9454_set_pins(pca[id].dev, pca[id].out);
}

void msg_pca9454(int param, const char* message, size_t message_len)
{
    int n = 0;
    int i, v;
    
    n = sscanf(message, "set:%d:%d", &i, &v);
    if(n == 2)
    {
        DBG("pca[%d]: %d -> %d", param, i, v);
        set_pca9454(param, i, v);
        return;
    }
}

