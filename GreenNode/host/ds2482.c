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

#define OW_CMD_SKIP             0xCC
#define OW_CMD_SELECT           0x55
#define OW_CMD_SEARCH           0xF0

typedef struct
{
    uint8_t addr[8];
    uint8_t next;
} ow_serch_t;


int ds2482_open(uint8_t bus, uint8_t adr)
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

int ds2482_set_conf(int dev, uint8_t conf)
{
    union i2c_smbus_data data;
    data.byte = ((~conf & 0x0F) << 4) | (conf & 0x0F);
    return i2c_io(dev, I2C_SMBUS_WRITE, 0xD2, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_reset(int dev)
{
    int res = -1;
    do
    {
        res = i2c_io(dev, I2C_SMBUS_WRITE, 0xF0, I2C_SMBUS_BYTE, NULL);
        if(res < 0) break;
        usleep(20);
        res = ds2482_set_conf(dev, 0x00);
    } while(0);
    return res;
}

int ds2482_get_status(int dev, uint8_t* value)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xF0;
        res = i2c_io(dev, I2C_SMBUS_WRITE, 0xE1, I2C_SMBUS_BYTE_DATA, &data);
        if(res < 0) break;
        res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *value = 0x0FF & data.byte;
    } while(0);
    return res;
}

int ds2482_get_data(int dev, uint8_t* value)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xE1;
        res = i2c_io(dev, I2C_SMBUS_WRITE, 0xE1, I2C_SMBUS_BYTE_DATA, &data);
        if(res < 0) break;
        res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *value = 0x0FF & data.byte;
    } while(0);
    return res;
}

int ds2482_get_conf(int dev, uint8_t* conf)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xC3;
        res = i2c_io(dev, I2C_SMBUS_WRITE, 0xE1, I2C_SMBUS_BYTE_DATA, &data);
        if(res < 0) break;
        res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *conf = 0x0FF & data.byte;
    } while(0);
    return res;
}

int ds2482_read(int dev, uint8_t* value)
{
    union i2c_smbus_data data;
    int res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
    *value = 0x0FF & data.byte;
    return res;
}

int ds2482_1w_reset(int dev)
{
    return i2c_io(dev, I2C_SMBUS_WRITE, 0xB4, I2C_SMBUS_BYTE, NULL);
}

int ds2482_1w_1b(int dev, char bit)
{
    union i2c_smbus_data data;
    data.byte = bit ? 0x80 : 0x00;
    return i2c_io(dev, I2C_SMBUS_WRITE, 0x87, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_1w_wbyte(int dev, uint8_t value)
{
    union i2c_smbus_data data;
    data.byte = value;
    return i2c_io(dev, I2C_SMBUS_WRITE, 0xA5, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_1w_rbyte(int dev)
{
    return i2c_io(dev, I2C_SMBUS_WRITE, 0x96, I2C_SMBUS_BYTE, NULL);
}

int ds2482_pool(int dev, uint8_t mask, uint8_t* value, int count)
{
    int res = -1, n = 0;
    union i2c_smbus_data data;
    do
    {
        if(n++ > count) { res = -2; break; }
        usleep(1);
        res = i2c_io(dev, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
    }
    while((data.byte & mask) != *value);
    *value = data.byte;
    return res;
}

int ds2482_1w_triplet(int dev, char bit)
{
    union i2c_smbus_data data;
    data.byte = bit ? 0x80 : 0x00;
    return i2c_io(dev, I2C_SMBUS_WRITE, 0x78, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_1w_search(int dev, ow_serch_t* search)
{
    int res = -1;
    uint8_t id, dir, comp, status, i, zero = 0;

    if(search->next == 0xFF) return -2;

    do
    {
        res = ds2482_1w_wbyte(dev, OW_CMD_SEARCH);
        if(res < 0) break;

        for(i = 0; i< 64; ++i)
        {
            int n = i/8;
            int b = 1 << (i%8);

            if(i < search->next) dir = search->addr[n] & b;
            else dir = i == search->next;

            status = 0;
            res = ds2482_pool(dev, 0x01, &status, 2000);
            if(res < 0) break;

            res = ds2482_1w_triplet(dev, dir);

            status = 0;
            res = ds2482_pool(dev, 0x01, &status, 2000);
            if(res < 0) break;

            id = status & 0x20;
            comp = status & 0x40;
            dir = status & 0x80;

            if(id && comp) { res = -2; break; }
            else if(!id && !comp && !dir) zero = i;

            if(dir) search->addr[n] |= b;
            else search->addr[n] &= ~b;
        }
        if(res < 0) break;

        if(zero == 0) search->next = 0xFF;
        else search->next = zero;

    } while(0);

    return res;
}

#if 1
int main(int argc, char* argv[])
{
    int res;
    uint8_t status;
    ow_serch_t search;
    int dev = ds2482_open(0, 0x18);

    if(dev < 0)
    {
        printf("Error: ds2482_open() failed %d!\n", errno);
        exit(1);
    }

    res = ds2482_reset(dev);
    if(res < 0)
    {
        printf("Error: ds2482_reset() failed %d!\n", errno);
        close(dev);
        exit(1);
    }

    status = 0;
    res = ds2482_pool(dev, 0x10, &status, 4000);
    if(res < 0)
    {
        printf("Error: ds2482_pool(0x10) failed %d!\n", errno);
        close(dev);
        exit(1);
    }

    res = ds2482_1w_reset(dev);
    if(res < 0)
    {
        printf("Error: ds2482_1w_reset() failed %d!\n", errno);
        close(dev);
        exit(1);
    }

    status = 0;
    res = ds2482_pool(dev, 0x01, &status, 3000);
    if(res < 0)
    {
        printf("Error: ds2482_pool(0x01) failed %d!\n", errno);
        close(dev);
        exit(1);
    }

    if(status & 0x04)
    {
        printf("Error: 1W short detected 0x%02X!\n", status);
        close(dev);
        exit(1);
    }

    if(status & 0x02 == 0)
    {
        printf("Error: 1W presence pulse is not detected 0x%02X!\n", status);
        close(dev);
        exit(1);
    }

    res = ds2482_set_conf(dev, 0x01); // Enable APU
    if(res < 0)
    {
        printf("Error: ds2482_set_conf() failed %d!\n", errno);
        close(dev);
        exit(1);
    }

    memset(&search, 0, sizeof(search));

    do
    {
        res = ds2482_1w_search(dev, &search);
        if(res < 0) break;

        printf("Found 1W device: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n", search.addr[0], search.addr[1], search.addr[2], search.addr[3], search.addr[4], search.addr[5], search.addr[6], search.addr[7]);
    } while(1);

    close(dev);

    return 0;
}
#endif
