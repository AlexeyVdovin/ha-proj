#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>
#include <unistd.h>

#include "config.h"
#include "uplink.h"
#include "boiler.h"

#include "ds2482.h"

#define STM_EXT_RST 0x8000
#define STM_DCDC_EN 0x0100
#define STM_LED_ON	0x0001

#define STM_HEARTBEAT 0x00
#define STM_STATUS    0x08
#define STM_CONTROL   0x0C
#define STM_DC_12V    0x0E
#define STM_ADC_IN1   0x10
#define STM_ADC_IN2   0x12
#define STM_DC_REF    0x18
#define STM_TEMPR     0x1A
#define STM_PWM1      0x1C
#define STM_PWM2      0x1E
#define STM_OW_CH     0x20

enum
{
    ST_IDLE = 0,
    ST_DC_12V,
    ST_ADC_IN1,
    ST_ADC_IN2,
    ST_IN_VDD,
    ST_OW_CH1_ENUM,
    ST_OW_CH1_READ,
    ST_OW_CH2_ENUM,
    ST_OW_CH2_READ,
    ST_OW_CH3_ENUM,
    ST_OW_CH3_READ,
    ST_OW_CH4_ENUM,
    ST_OW_CH4_READ
};

static const char STR_DC_12V[]      = "bl:12V";
static const char STR_ADC_IN1[]     = "bl:pressure1";
static const char STR_ADC_IN2[]     = "bl:pressure1";
static const char STR_VDD_OK[]      = "bl:VDD";
static const char STR_OW_CH1_ENUM[] = "bl:ch1";
static const char STR_OW_CH2_ENUM[] = "bl:ch2";
static const char STR_OW_CH3_ENUM[] = "bl:ch3";
static const char STR_OW_CH4_ENUM[] = "bl:ch4";


typedef struct 
{
    uint8_t addr[8];
    float   t;
} ow_t;


typedef struct
{
    int      dev;
    int      ds2482;
    int      n;
    int      state;
    uint16_t control;
    uint16_t ow_ch;
    int      ch1_n;
    ow_t     ch1[OW_MAX_DEVCES];
    int      ch2_n;
    ow_t     ch2[OW_MAX_DEVCES];
    int      ch3_n;
    ow_t     ch3[OW_MAX_DEVCES];
    int      ch4_n;
    ow_t     ch4[OW_MAX_DEVCES];
} boil_t;

boil_t boiler;

static int boiler_i2c_open(uint8_t bus, uint8_t adr)
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

static inline int i2c_io(int dev, char read_write, uint8_t command, int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(dev, I2C_SMBUS, &args);
}

static int stm_set_control(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_CONTROL, I2C_SMBUS_WORD_DATA, &data);
}

static int stm_set_ow_ch(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_OW_CH, I2C_SMBUS_WORD_DATA, &data);
}

static int stm_get_dc(int dev, uint8_t in, uint16_t *val)
{
    int res;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, in, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;
    
    return res;
}

static int stm_get_vdd(int dev, uint16_t *val)
{
    int res;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, STM_STATUS, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;
    
    return res;
}

static int boiler_ch_enum(uint16_t ch, uint16_t *val)
{
    int res, i, n = 0, dev = boiler.ds2482;
    uint8_t status;
    ow_serch_t search;
    uint16_t en = 0;
    ow_t* ow;

    switch(ch)
    {
        case 1:
            en = 0x0001;
            ow = boiler.ch1;
            break;
        case 2:
            en = 0x0002;
            ow = boiler.ch2;
            break;
        case 3:
            en = 0x0004;
            ow = boiler.ch3;
            break;
        case 4:
            en = 0x0008;
            ow = boiler.ch3;
            break;
        default:
            return -1;
    }

    do
    {
        res = stm_set_ow_ch(boiler.dev, (en & 0x0F));
        if(res < 0)
        {
            DBG("Error: stm_set_ow_ch() failed %d!\n", errno);
            break;
        }
        res = ds2482_reset(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_reset() failed %d!\n", errno);
            status = 0;
            res = ds2482_get_status(dev, &status);
            DBG("Error: ds2482_get_status() = %d, status = %d\n", res, status);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x10, &status, 4000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x10) failed %d!\n", errno);
            break;
        }

        memset(&search, 0, sizeof(search));

        do
        {
            res = ds2482_set_conf(dev, 0x00); // Disable APU
            if(res < 0)
            {
                DBG("Error: ds2482_set_conf() failed %d!\n", errno);
                break;
            }

            res = ds2482_1w_reset(dev);
            if(res < 0)
            {
                DBG("Error: ds2482_1w_reset() failed %d!\n", errno);
                break;
            }

            status = 0;
            res = ds2482_pool(dev, 0x01, &status, 3000);
            if(res < 0)
            {
                DBG("Error: ds2482_pool(0x01) failed %d!\n", errno);
                break;
            }

            if(status & 0x04)
            {
                DBG("Error: 1W short to GND detected 0x%02X!\n", status);
                break;
            }

            if(status & 0x02 == 0)
            {
                DBG("Error: 1W presence pulse is not detected 0x%02X!\n", status);
                break;
            }

            res = ds2482_set_conf(dev, 0x01); // Enable APU
            if(res < 0)
            {
                DBG("Error: ds2482_set_conf() failed %d!\n", errno);
                break;
            }

            res = ds2482_1w_search(dev, &search);
            if(res < 0)
            {
                res = 0; 
                *val = n;
                break;
            }

            DBG("Found 1W device [%d]: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n", n, search.addr[0], search.addr[1], search.addr[2], search.addr[3], search.addr[4], search.addr[5], search.addr[6], search.addr[7]);
            memcpy(ow[n].addr, search.addr, sizeof(ow[n].addr));
            ++n;
        } while(1);
    } while(0);
    return res;
}

static int boiler_ch_measure()
{
    int res, dev = boiler.ds2482;
    uint8_t status;
    uint8_t data[9];

    do
    {
        res = ds2482_1w_reset(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_reset() failed %d!\n", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 3000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!\n", errno);
            break;
        }

        if(status & 0x04)
        {
            DBG("Error: 1W short to GND detected 0x%02X!\n", status);
            break;
        }

        if(status & 0x02 == 0)
        {
            DBG("Error: 1W presence pulse is not detected 0x%02X!\n", status);
            break;
        }

        res = ds2482_1w_skip(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_skip() failed %d!\n", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 2000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!\n", errno);
            break;
        }

        res = ds2482_ds18b20_convert(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_ds18b20_convert() failed %d!\n", errno);
            break;
        }
    } while(0);
    return res;
}

static int boiler_ch_read(uint8_t* addr, int16_t *value)
{
    int res, dev = boiler.ds2482;
    uint8_t status;
    uint8_t data[9];

    do
    {
        res = ds2482_1w_reset(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_reset() failed %d!\n", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 3000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!\n", errno);
            break;
        }

        if(status & 0x04)
        {
            DBG("Error: 1W short to GND detected 0x%02X!\n", status);
            break;
        }

        if(status & 0x02 == 0)
        {
            DBG("Error: 1W presence pulse is not detected 0x%02X!\n", status);
            break;
        }

        res = ds2482_1w_match(dev, addr);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_match() failed %d!\n", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 2000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!\n", errno);
            break;
        }

        res = ds2482_ds18b20_read_scratchpad(dev, data);
        if(res < 0)
        {
            DBG("Error: ds2482_ds18b20_read_scratchpad() failed %d!\n", errno);
            break;
        }

        *value = (data[1] << 8) | data[0];   
    } while(0);
    return res;
}


void handle_boiler()
{
    static int s = -20;
    int i, t, res;
    uint8_t c;
    uint16_t val;
    int16_t value;
    const char* event;
    char str[16];
    
    t = time(0);
    if(t != s)
    {
        s = t;
        // DBG("stm: %d", t%60);
        switch(t%60)
        {
            case ST_DC_12V:
            {
                event = STR_DC_12V;
                stm_get_dc(boiler.dev, STM_DC_12V, &val);
                sprintf(str, "%d", val);
                break;
            }
            case ST_ADC_IN1:
            {
                event = STR_ADC_IN1;
                stm_get_dc(boiler.dev, STM_ADC_IN1, &val);
                sprintf(str, "%d", val);
                break;
            }
            case ST_ADC_IN2:
            {
                event = STR_ADC_IN2;
                stm_get_dc(boiler.dev, STM_ADC_IN2, &val);
                sprintf(str, "%d", val);
                break;
            }
            case ST_IN_VDD:
            {
                event = STR_VDD_OK;
                stm_get_vdd(boiler.dev, &val);
                sprintf(str, "%s", val ? "ON" : "OFF");
                break;
            }
            case ST_OW_CH1_ENUM:
            {
                event = STR_OW_CH1_ENUM;
                val = 0;
                boiler_ch_enum(1, &val);
                boiler.ch1_n = val;
                boiler_ch_measure();
                break;
            }
            case ST_OW_CH1_READ:
            {
                t = 0;
                for(i = 0; i < boiler.ch1_n; ++i)
                {
                    boiler.ch1[i].t = -200;
                    res = boiler_ch_read(boiler.ch1[i].addr, &value);
                    if(res < 0) continue;
                    boiler.ch1[i].t = value/16.0f;
                    DBG("ch1: %d = %fC", i, boiler.ch1[i].t);
                }
                break;
            }
            case ST_OW_CH2_ENUM:
            {
                event = STR_OW_CH2_ENUM;
                val = 0;
                boiler_ch_enum(2, &val);
                boiler.ch2_n = val;
                boiler_ch_measure();
                break;
            }
            case ST_OW_CH2_READ:
            {
                t = 0;
                for(i = 0; i < boiler.ch2_n; ++i)
                {
                    boiler.ch2[i].t = -200;
                    res = boiler_ch_read(boiler.ch2[i].addr, &value);
                    if(res < 0) continue;
                    boiler.ch2[i].t = value/16.0f;
                    DBG("ch2: %d = %fC", i, boiler.ch2[i].t);
                }
                break;
            }
            case ST_OW_CH3_ENUM:
            {
                event = STR_OW_CH3_ENUM;
                val = 0;
                boiler_ch_enum(3, &val);
                boiler.ch3_n = val;
                boiler_ch_measure();
                break;
            }
            case ST_OW_CH3_READ:
            {
                t = 0;
                for(i = 0; i < boiler.ch3_n; ++i)
                {
                    boiler.ch3[i].t = -200;
                    res = boiler_ch_read(boiler.ch3[i].addr, &value);
                    if(res < 0) continue;
                    boiler.ch3[i].t = value/16.0f;
                    DBG("ch3: %d = %fC", i, boiler.ch3[i].t);
                }
                break;
            }
            case ST_OW_CH4_ENUM:
            {
                event = STR_OW_CH4_ENUM;
                val = 0;
                boiler_ch_enum(4, &val);
                boiler.ch4_n = val;
                boiler_ch_measure();
                break;
            }
            case ST_OW_CH4_READ:
            {
                t = 0;
                for(i = 0; i < boiler.ch4_n; ++i)
                {
                    boiler.ch4[i].t = -200;
                    res = boiler_ch_read(boiler.ch4[i].addr, &value);
                    if(res < 0) continue;
                    boiler.ch4[i].t = value/16.0f;
                    DBG("ch4: %d = %fC", i, boiler.ch4[i].t);
                }
                break;
            }
            default:
            {
                t = 0;
                break;
            }
        }
        if(t)
        {
            send_mqtt(event, str);
        }
    }
}

void setup_boiler_poll()
{
}

void init_boiler()
{
    int v, res;
    int dev;
    
    memset(&boiler, 0, sizeof(boiler));
    boiler.dev = -1;
    boiler.ds2482 = -1;

    dev = boiler_i2c_open(0, 0x40 + cfg.boiler.id);

    if(dev < 0)
    {
        DBG("Error: boiler_i2c_open(0x%02x) failed %d!", 0x40 + cfg.boiler.id, errno);
        return;
    }
    boiler.dev = dev;

    res = stm_set_control(dev, boiler.control);
    res = stm_set_ow_ch(dev, boiler.ow_ch);

    dev = ds2482_open(0, 0x18 + cfg.boiler.id);
    if(dev < 0)
    {
        DBG("Error: boiler_i2c_open(0x%02x) failed %d!", 0x18 + cfg.boiler.id, errno);
        return;
    }
    boiler.ds2482 = dev;

}

void close_boiler()
{
    if(boiler.dev > 0) close(boiler.dev);   
    boiler.dev = -1;
}
