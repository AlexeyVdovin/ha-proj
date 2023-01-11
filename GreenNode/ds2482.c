#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#include "config.h"
#include "ds2482.h"

typedef struct
{
    int      dev;
} ds2482_t;

ds2482_t ds;


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
            DBG("Error %d: Can't open i2c device %s\n", errno, devname);
            break;
        }

        res = ioctl(dev, I2C_SLAVE, adr);

        if(res < 0)
        {
            close(dev);
            dev = -1;
            DBG("Error %d: Can't assign slave i2c address.\n", errno);
            break;
        }

    } while(0);

    return dev;
}

static inline int i2c_io(char read_write, uint8_t command, int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;
    int dev = ds.dev;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(dev, I2C_SMBUS, &args);
}

int ds2482_set_conf(uint8_t conf)
{
    union i2c_smbus_data data;
    data.byte = ((~conf & 0x0F) << 4) | (conf & 0x0F);
    return i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_WRITECONFIG, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_reset()
{
    int res = -1;
    do
    {
        res = i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_RESET, I2C_SMBUS_BYTE, NULL);
        if(res < 0) break;
        usleep(1000);
        res = ds2482_set_conf(0x00);
    } while(0);
    return res;
}

int ds2482_get_status(uint8_t* value)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xF0;
        res = i2c_io(I2C_SMBUS_WRITE, 0xE1, I2C_SMBUS_BYTE_DATA, &data);
        if(res < 0) break;
        res = i2c_io(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *value = 0x0FF & data.byte;
    } while(0);
    return res;
}

int ds2482_get_data(uint8_t* value)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xE1;
        res = i2c_io(I2C_SMBUS_WRITE, 0xE1, I2C_SMBUS_BYTE_DATA, &data);
        if(res < 0) break;
        res = i2c_io(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *value = (0x0FF & data.byte);
    } while(0);
    return res;
}

int ds2482_get_conf(uint8_t* conf)
{
    int res = -1;
    union i2c_smbus_data data;
    do
    {
        data.byte = 0xC3;
        res = i2c_io(I2C_SMBUS_WRITE, 0xE1, I2C_SMBUS_BYTE_DATA, &data);
        if(res < 0) break;
        res = i2c_io(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
        *conf = 0x0FF & data.byte;
    } while(0);
    return res;
}

int ds2482_read(uint8_t* value)
{
    union i2c_smbus_data data;
    int res = i2c_io(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
    *value = 0x0FF & data.byte;
    return res;
}

int ds2482_1w_reset()
{
    return i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_RESETWIRE, I2C_SMBUS_BYTE, NULL);
}

int ds2482_1w_1b(char bit)
{
    union i2c_smbus_data data;
    data.byte = bit ? 0x80 : 0x00;
    return i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_SINGLEBIT, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_1w_wbyte(uint8_t value)
{
    union i2c_smbus_data data;
    data.byte = value;
    return i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_WRITEBYTE, I2C_SMBUS_BYTE_DATA, &data);
}

int ds2482_1w_rbyte()
{
    return i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_READBYTE, I2C_SMBUS_BYTE, NULL);
}

int ds2482_pool(uint8_t mask, uint8_t* value, int count)
{
    int res = -1, n = 0;
    union i2c_smbus_data data;
    do
    {
        if(n++ > count) { res = -2; break; }
        usleep(1);
        res = i2c_io(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
        if(res < 0) break;
    }
    while((data.byte & mask) != *value);
    *value = data.byte;
    return res;
}

int ds2482_1w_triplet(char bit)
{
    union i2c_smbus_data data;
    data.byte = bit ? 0x80 : 0x00;
    return i2c_io(I2C_SMBUS_WRITE, DS2482_CMD_TRIPLET, I2C_SMBUS_BYTE_DATA, &data);
}

/* I2C gateway 1W high level operations */
int ds2482_1w_search(ow_serch_t* search)
{
    int res = -1;
    uint8_t id, dir, comp, status, i, zero = 0;

    if(search->next == 0xFF) return -2;

    do
    {
        res = ds2482_1w_wbyte(OW_CMD_SEARCH);
        if(res < 0) break;

        for(i = 0; i< 64; ++i)
        {
            int n = i/8;
            int b = 1 << (i%8);

            if(i < search->next) dir = search->addr[n] & b;
            else dir = i == search->next;

            status = 0;
            res = ds2482_pool(0x01, &status, 2000);
            if(res < 0) break;

            res = ds2482_1w_triplet(dir);
            if(res < 0) break;

            status = 0;
            res = ds2482_pool(0x01, &status, 2000);
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

int ds2482_1w_skip()
{
    return ds2482_1w_wbyte(OW_CMD_SKIP);
}

int ds2482_1w_match(uint8_t addr[8])
{
    int i, res;
    uint8_t status;

    do
    {
        res = ds2482_1w_wbyte(OW_CMD_MATCH);
        if(res < 0) break;

        for(i = 0; i < 8; ++i)
        {
            status = 0;
            res = ds2482_pool(0x01, &status, 2000);
            if(res < 0) break;

            res = ds2482_1w_wbyte(addr[i]);
            if(res < 0) break;
        }
    } while(0);

    return res;
}

/* I2C gateway DS18B20 operations */
int ds2482_ds18b20_convert()
{
    return ds2482_1w_wbyte(DS1820_CMD_CONVERT);
}

int ds2482_ds18b20_read_power(uint8_t* value)
{
    int res;
    uint8_t status;

    do
    {
        res = ds2482_1w_wbyte(DS1820_CMD_PPD);
        if(res < 0) break;

        status = 0;
        res = ds2482_pool(0x01, &status, 2000);
        if(res < 0) break;

        res = ds2482_1w_1b(0x80);
        if(res < 0) break;

        status = 0;
        res = ds2482_pool(0x01, &status, 2000);
        if(res < 0) break;

        *value = status;
    } while(0);

    return res;
}

int ds2482_ds18b20_read_scratchpad(uint8_t* data)
{
    int i, res;
    uint8_t status;

    do
    {
        res = ds2482_1w_wbyte(DS1820_CMD_RD_SCRATCHPAD);
        if(res < 0) break;

        status = 0;
        res = ds2482_pool(0x01, &status, 2000);
        if(res < 0) break;

        for(i = 0; i < 9; ++i)
        {
            res = ds2482_1w_rbyte();
            if(res < 0) break;

            status = 0;
            res = ds2482_pool(0x01, &status, 2000);
            if(res < 0) break;

            res = ds2482_get_data(&data[i]);
            if(res < 0) break;

            // DBG("scr[%d]: 0x%02x\n", i, data[i]);
        }
        // TODO: Check CRC
    } while(0);

    return res;
}

int ds2482_ds18b20_read_eeprom()
{
    return ds2482_1w_wbyte(DS1820_CMD_EE_RECALL);
}

int ds2482_ds18b20_write_configuration(uint8_t* conf)
{
    int i, res;
    uint8_t status;

    do
    {
        res = ds2482_1w_wbyte(DS1820_CMD_WR_SCRATCHPAD);
        if(res < 0) break;

        for(i = 0; i < 3; ++i)
        {
            status = 0;
            res = ds2482_pool(0x01, &status, 2000);
            if(res < 0) break;

            res = ds2482_1w_wbyte(conf[i]);
            if(res < 0) break;
        }
    } while(0);

    return res;
}

int ds2482_ds18b20_write_eeprom()
{
    return ds2482_1w_wbyte(DS1820_CMD_EE_WRITE);
}

int ds2482_ds18b20_scan_temp()
{
    int res, i, n = 0;
    uint8_t status;
    ow_serch_t search;
    uint8_t addr[8][8];

    res = ds2482_reset();
    if(res < 0)
    {
        DBG("Error: ds2482_reset() failed %d!", errno);
        status = 0;
        res = ds2482_get_status(&status);
        DBG("Error: ds2482_get_status() = %d, status = %d", res, status);
        return res;
    }

    status = 0;
    res = ds2482_pool(0x10, &status, 4000);
    if(res < 0)
    {
        DBG("Error: ds2482_pool(0x10) failed %d!", errno);
        return res;
    }

    memset(&search, 0, sizeof(search));

    do
    {
        res = ds2482_set_conf(0x00); // Disable APU
        if(res < 0)
        {
            DBG("Error: ds2482_set_conf() failed %d!", errno);
            break;
        }

        res = ds2482_1w_reset();
        if(res < 0)
        {
            DBG("Error: ds2482_1w_reset() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(0x01, &status, 3000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        if(status & 0x04)
        {
            DBG("Error: 1W short to GND detected 0x%02X!", status);
            break;
        }

        if(status & 0x02 == 0)
        {
            DBG("Error: 1W presence pulse is not detected 0x%02X!", status);
            break;
        }

        res = ds2482_set_conf(0x01); // Enable APU
        if(res < 0)
        {
            DBG("Error: ds2482_set_conf() failed %d!", errno);
            break;
        }

        res = ds2482_1w_search(&search);
        if(res < 0) break;

        DBG("Found 1W device [%d]: 0x%02x%02x%02x%02x%02x%02x%02x%02x", n, search.addr[0], search.addr[1], search.addr[2], search.addr[3], search.addr[4], search.addr[5], search.addr[6], search.addr[7]);
        memcpy(addr[n], search.addr, sizeof(addr[0]));
        ++n;
    } while(1);

    if(n == 0)
    {
        DBG("Error: No 1W devices found!\n");
        return res;
    }

    do
    {
        uint8_t data[9];

        res = ds2482_1w_reset();
        if(res < 0)
        {
            DBG("Error: ds2482_1w_reset() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(0x01, &status, 3000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        if(status & 0x04)
        {
            DBG("Error: 1W short to GND detected 0x%02X!", status);
            break;
        }

        if(status & 0x02 == 0)
        {
            DBG("Error: 1W presence pulse is not detected 0x%02X!", status);
            break;
        }

        res = ds2482_1w_skip();
        if(res < 0)
        {
            DBG("Error: ds2482_1w_match() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(0x01, &status, 2000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        res = ds2482_ds18b20_convert();
        if(res < 0)
        {
            DBG("Error: ds2482_ds18b20_convert() failed %d!", errno);
            break;
        }

        sleep(1);
    } while(0);

    if(res < 0)
    {
        return res;
    }

    for(i = 0; i < n; ++i)
    {
        uint8_t data[9];

        res = ds2482_1w_reset();
        if(res < 0)
        {
            DBG("Error: ds2482_1w_reset() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(0x01, &status, 3000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        if(status & 0x04)
        {
            DBG("Error: 1W short to GND detected 0x%02X!", status);
            break;
        }

        if(status & 0x02 == 0)
        {
            DBG("Error: 1W presence pulse is not detected 0x%02X!", status);
            break;
        }

        res = ds2482_1w_match(addr[i]);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_match() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(0x01, &status, 2000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        res = ds2482_ds18b20_read_scratchpad(data);
        if(res < 0)
        {
            DBG("Error: ds2482_ds18b20_read_scratchpad() failed %d!", errno);
            break;
        }

        int16_t t = (data[1] << 8) | data[0];
        DBG("Read temperature: 0x%02x 0x%02x, %.4f", data[0], data[1], t/16.0);

        DBG("0x%02x%02x%02x%02x%02x%02x%02x%02x %d\n", addr[i][0], addr[i][1], addr[i][2], addr[i][3], addr[i][4], addr[i][5], addr[i][6], addr[i][7], (int)(t*125/2));
    }
    return res;
}

void init_ds2482()
{
    int dev;

    dev = ds2482_open(0, DS2482_SLAVE_ADDR);

    if(dev < 0)
    {
        DBG("Error: stm_open() failed %d!", errno);
        return;
    }
    ds.dev = dev;

}

void close_ds2482()
{
    if(ds.dev > 0) close(ds.dev);   
    ds.dev = -1;
}
