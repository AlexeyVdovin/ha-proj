#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <time.h>

#include "config.h"
#include "uplink.h"
#include "boiler.h"

#include "ds2482.h"

ha_device_t device =
{
    "Heating Controller",
    "Heating Controller",
    "WallSW 0.0.1"
};

// Topic: "homeassistant/sensor/heating_pressure1/config"
/* Resulted JSON:
{
  "name": "Cold Water",
  "unique_id": "heating_pressure1",
  "state_topic": "homeassistant/sensor/heating_pressure1/state",
  "device_class": "pressure",
  "unit_of_measurement": "Bar",
  "platform": "mqtt",
  "device": {
    "identifiers": ["Heating Controller"],
    "name": "Heating Controller",
    "manufacturer": "AtHome",
    "model": "Heating Controller",
    "sw_version": "WallSW 0.0.1"
  }
} */
ha_entity_t en_pressure1 =
{
    "heating_pressure1",
    "sensor",
    "Cold Water",
    "pressure",
    "Bar"
};

ha_entity_t en_pressure2 =
{
    "heating_pressure2",
    "sensor",
    "Coolant Water",
    "pressure",
    "Bar"
};

ha_entity_t en_t_boiler1 =
{
    "boiler_temperature1",
    "sensor",
    "Boiler 1",
    "temperature",
    "°C"
};

ha_entity_t en_t_boiler2 =
{
    "boiler_temperature2",
    "sensor",
    "Boiler 2",
    "temperature",
    "°C"
};

ha_entity_t en_t_boiler_in =
{
    "boiler_temperature_in",
    "sensor",
    "Boiler Inlet",
    "temperature",
    "°C"
};

ha_entity_t en_t_boiler_out =
{
    "boiler_temperature_out",
    "sensor",
    "Boiler Outlet",
    "temperature",
    "°C"
};

ha_entity_t en_t_boiler_ret =
{
    "boiler_temperature_ret",
    "sensor",
    "Boiler Circulation",
    "temperature",
    "°C"
};

ha_entity_t en_t_floor_in =
{
    "ht_floor_temperature_in",
    "sensor",
    "Heating Floor in",
    "temperature",
    "°C"
};

ha_entity_t en_t_floor_out =
{
    "ht_floor_temperature_out",
    "sensor",
    "Heating Floor out",
    "temperature",
    "°C"
};

ha_entity_t en_t_floor_ret =
{
    "ht_floor_temperature_ret",
    "sensor",
    "Heating Floor ret",
    "temperature",
    "°C"
};

ha_entity_t en_t_heating_out =
{
    "heating_temperature_out",
    "sensor",
    "Heating out",
    "temperature",
    "°C"
};

ha_entity_t en_t_heating_ret =
{
    "heating_temperature_ret",
    "sensor",
    "Heating ret",
    "temperature",
    "°C"
};

ha_entity_t en_t_gas_heater_in =
{
    "gas_heater_temperature_in",
    "sensor",
    "Gas Heater in",
    "temperature",
    "°C"
};

ha_entity_t en_t_gas_heater_out =
{
    "gas_heater_temperature_out",
    "sensor",
    "Gas Heater out",
    "temperature",
    "°C"
};

ha_entity_t en_t_gas_boiler_in =
{
    "gas_boiler_temperature_in",
    "sensor",
    "Gas Boiler in",
    "temperature",
    "°C"
};

ha_entity_t en_t_gas_boiler_out =
{
    "gas_boiler_temperature_out",
    "sensor",
    "Gas Boiler out",
    "temperature",
    "°C"
};

ha_entity_t en_t_ambient =
{
    "boiler_temperature_ambient",
    "sensor",
    "Ambient",
    "temperature",
    "°C"
};


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
    ST_OW_CH4_READ,
    ST_CYCLE_CONTROL,
    ST_CYCLE_START,
    ST_CYCLE_END = 30
};

enum
{
    ST_CTL_CH1 = 0x01,
    ST_CTL_CH2 = 0x02,
    ST_CTL_CH3 = 0x04,
    ST_CTL_CH4 = 0x08,
    ST_CTL_CH5 = 0x10,
    ST_CTL_CH6 = 0x20
};

static const char STR_DC_12V[]      = "bl:12V";
static const char STR_ADC_IN1[]     = "bl:pressure1";
static const char STR_ADC_IN2[]     = "bl:pressure2";
static const char STR_VDD_OK[]      = "bl:VDD";
static const char STR_OW_CH1_ENUM[] = "bl:ch1";
static const char STR_OW_CH2_ENUM[] = "bl:ch2";
static const char STR_OW_CH3_ENUM[] = "bl:ch3";
static const char STR_OW_CH4_ENUM[] = "bl:ch4";


typedef struct 
{
    uint8_t addr[8];
    int     type;
    float   t;
} ow_t;

typedef struct {
  int16_t p;   // Proportional gain
  int16_t i;   // Integral gain
  int32_t val; // control Value
  float   d;   // Derivative
  float   t;   // previouse Temp
  float   integral;
} pd_t;

typedef struct
{
    int      dev;
    int      ds2482;
    int      n_fd;
    int      tmr;
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
    float    hwater1;
    float    hwater2;
    float    ht_floor_out;
    float    ht_floor_ret;
    float    ht_floor_set;
    pd_t     pid1;
    struct itimerspec tmr_value;
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
            DBG("Error %d: Can't open i2c device %s", errno, devname);
            break;
        }

        res = ioctl(dev, I2C_SLAVE, adr);

        if(res < 0)
        {
            close(dev);
            dev = -1;
            DBG("Error %d: Can't assign slave i2c address.", errno);
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

static int boiler_set_pwm1(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_PWM1, I2C_SMBUS_WORD_DATA, &data);
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
            ow = boiler.ch4;
            break;
        default:
            return -1;
    }

    do
    {
        res = stm_set_ow_ch(boiler.dev, (en & 0x0F));
        if(res < 0)
        {
            DBG("Error: stm_set_ow_ch() failed %d!", errno);
            break;
        }
        res = ds2482_reset(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_reset() failed %d!", errno);
            status = 0;
            res = ds2482_get_status(dev, &status);
            DBG("Error: ds2482_get_status() = %d, status = %d", res, status);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x10, &status, 4000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x10) failed %d!", errno);
            break;
        }

        memset(&search, 0, sizeof(search));

        do
        {
            res = ds2482_set_conf(dev, 0x00); // Disable APU
            if(res < 0)
            {
                DBG("Error: ds2482_set_conf() failed %d!", errno);
                break;
            }

            res = ds2482_1w_reset(dev);
            if(res < 0)
            {
                DBG("Error: ds2482_1w_reset() failed %d!", errno);
                break;
            }

            status = 0;
            res = ds2482_pool(dev, 0x01, &status, 3000);
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

            res = ds2482_set_conf(dev, 0x01); // Enable APU
            if(res < 0)
            {
                DBG("Error: ds2482_set_conf() failed %d!", errno);
                break;
            }

            res = ds2482_1w_search(dev, &search);
            if(res < 0)
            {
                res = 0; 
                *val = n;
                break;
            }

            DBG("Found 1W device [%d]: 0x%02x%02x%02x%02x%02x%02x%02x%02x", n, search.addr[0], search.addr[1], search.addr[2], search.addr[3], search.addr[4], search.addr[5], search.addr[6], search.addr[7]);
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
            DBG("Error: ds2482_1w_reset() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 3000);
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

        res = ds2482_1w_skip(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_skip() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 2000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        res = ds2482_ds18b20_convert(dev);
        if(res < 0)
        {
            DBG("Error: ds2482_ds18b20_convert() failed %d!", errno);
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
            DBG("Error: ds2482_1w_reset() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 3000);
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

        res = ds2482_1w_match(dev, addr);
        if(res < 0)
        {
            DBG("Error: ds2482_1w_match() failed %d!", errno);
            break;
        }

        status = 0;
        res = ds2482_pool(dev, 0x01, &status, 2000);
        if(res < 0)
        {
            DBG("Error: ds2482_pool(0x01) failed %d!", errno);
            break;
        }

        res = ds2482_ds18b20_read_scratchpad(dev, data);
        if(res < 0)
        {
            DBG("Error: ds2482_ds18b20_read_scratchpad() failed %d!", errno);
            break;
        }

        *value = (data[1] << 8) | data[0];   
        // DBG("0x%02x%02x%02x%02x%02x%02x%02x%02x %d\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7], (int)((*value)*125/2));

    } while(0);
    return res;
}

/* p - pid*, set - target T, t - current T */
static int32_t update_pid(pd_t* p, float set, float t)
{
  float error = set - t;
  float delta = p->t - t;
  int32_t out = (int32_t)(p->p * error / 1024);
  out += (int32_t)(p->d * delta / 1024);
  p->integral += p->i * error / 1024;
  p->t = t;
  out += (int32_t)(p->integral);
  p->val = out;
  DBG("pid: Set=%.2f T=%.2f Out=%d Pint=%f", set, t, out, p->integral);
  return out;
}


static void boiler_hot_water()
{
    uint16_t control = boiler.control;
    uint16_t state = (control & ST_CTL_CH6);
    float t;

    if(boiler.hwater1 != -200) t = boiler.hwater1;
    if(boiler.hwater2 != -200) t = boiler.hwater2;

    if(t < cfg.boiler.hwater_min) state = 1;
    if(t > cfg.boiler.hwater_max) state = 0;

    control &= ~ST_CTL_CH6;
    control |= state ? ST_CTL_CH6 : 0;
    if(control != boiler.control)
    {
        stm_set_control(boiler.dev, control);
        boiler.control = control;
    }
}

static void boiler_pid_floor()
{
    float t;
    int32_t out = 0;
    uint16_t pwm = 0;

    if(boiler.ht_floor_out != -200 && boiler.ht_floor_ret != -200)
    {
        t = (boiler.ht_floor_out + boiler.ht_floor_ret)/2;
        out = update_pid(&boiler.pid1, boiler.ht_floor_set, t);
        pwm = (out > cfg.boiler.pwm1_max) ? cfg.boiler.pwm1_max : (out < cfg.boiler.pwm1_min) ? cfg.boiler.pwm1_min : out;
        boiler_set_pwm1(boiler.dev, pwm);
    }
}

static void tmrInterrupt0(void)
{
    static int s = -20;
    uint16_t val;
    int16_t value;
    const char* event;
    char str[32], topic[32];
    int res, i, t = time(0);

   // DBG("boiler: Tmr INT: %d", s);

    switch(s)
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
            for(i = 0; i < boiler.ch1_n; ++i)
            {
                int x;
                uint8_t* addr = boiler.ch1[i].addr;
                boiler.ch1[i].type = -1;
                sprintf(str, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
                for(x = 0; x < OW_RESERVED; ++x)
                {
                    if(strcmp(str, cfg.boiler.sensor[x]) == 0)
                    {
                        boiler.ch1[i].type = x;
                        break;
                    }
                }
            }
            sprintf(str, "%d", val);
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
                switch(boiler.ch1[i].type)
                {
                    case OW_BOILER_TEMP1:
                        boiler.hwater1 = value/16.0f;
                        break;
                    case OW_BOILER_TEMP2:
                        boiler.hwater2 = value/16.0f;
                        break;
                    case OW_FLOOR_OUT:
                        boiler.ht_floor_out = value/16.0f;
                        break;
                    case OW_FLOOR_RET:
                        boiler.ht_floor_ret = value/16.0f;
                        break;
                }
            }
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_OW_CH2_ENUM:
        {
            event = STR_OW_CH2_ENUM;
            val = 0;
            boiler_ch_enum(2, &val);
            boiler.ch2_n = val;
            boiler_ch_measure();
            for(i = 0; i < boiler.ch2_n; ++i)
            {
                int x;
                uint8_t* addr = boiler.ch2[i].addr;
                boiler.ch2[i].type = -1;
                sprintf(str, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
                for(x = 0; x < OW_RESERVED; ++x)
                {
                    if(strcmp(str, cfg.boiler.sensor[x]) == 0)
                    {
                        boiler.ch2[i].type = x;
                        break;
                    }
                }
            }
            sprintf(str, "%d", val);
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
                switch(boiler.ch2[i].type)
                {
                    case OW_BOILER_TEMP1:
                        boiler.hwater1 = value/16.0f;
                        break;
                    case OW_BOILER_TEMP2:
                        boiler.hwater2 = value/16.0f;
                        break;
                    case OW_FLOOR_OUT:
                        boiler.ht_floor_out = value/16.0f;
                        break;
                    case OW_FLOOR_RET:
                        boiler.ht_floor_ret = value/16.0f;
                        break;
                }
            }
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_OW_CH3_ENUM:
        {
            event = STR_OW_CH3_ENUM;
            val = 0;
            boiler_ch_enum(3, &val);
            boiler.ch3_n = val;
            boiler_ch_measure();
            for(i = 0; i < boiler.ch3_n; ++i)
            {
                int x;
                uint8_t* addr = boiler.ch3[i].addr;
                boiler.ch3[i].type = -1;
                sprintf(str, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
                for(x = 0; x < OW_RESERVED; ++x)
                {
                    if(strcmp(str, cfg.boiler.sensor[x]) == 0)
                    {
                        boiler.ch3[i].type = x;
                        break;
                    }
                }
            }
            sprintf(str, "%d", val);
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
                switch(boiler.ch3[i].type)
                {
                    case OW_BOILER_TEMP1:
                        boiler.hwater1 = value/16.0f;
                        break;
                    case OW_BOILER_TEMP2:
                        boiler.hwater2 = value/16.0f;
                        break;
                    case OW_FLOOR_OUT:
                        boiler.ht_floor_out = value/16.0f;
                        break;
                    case OW_FLOOR_RET:
                        boiler.ht_floor_ret = value/16.0f;
                        break;
                }
            }
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_OW_CH4_ENUM:
        {
            event = STR_OW_CH4_ENUM;
            val = 0;
            boiler_ch_enum(4, &val);
            boiler.ch4_n = val;
            boiler_ch_measure();
            for(i = 0; i < boiler.ch4_n; ++i)
            {
                int x;
                uint8_t* addr = boiler.ch4[i].addr;
                boiler.ch4[i].type = -1;
                sprintf(str, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
                for(x = 0; x < OW_RESERVED; ++x)
                {
                    if(strcmp(str, cfg.boiler.sensor[x]) == 0)
                    {
                        boiler.ch4[i].type = x;
                        break;
                    }
                }
            }
            sprintf(str, "%d", val);
            break;
        }
        case ST_OW_CH4_READ:
        {
            t = 0;
            float temp;
            for(i = 0; i < boiler.ch4_n; ++i)
            {
                uint8_t* addr = boiler.ch4[i].addr;
                boiler.ch4[i].t = -200;
                res = boiler_ch_read(boiler.ch4[i].addr, &value);
                if(res < 0) continue;
                temp = value/16.0f;
                boiler.ch4[i].t = temp;
                DBG("ch4: %d = %fC", i, boiler.ch4[i].t);
                sprintf(topic, "0x%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
                sprintf(str, "%.5f", temp);
                send_mqtt(topic, str);
                switch(boiler.ch4[i].type)
                {
                    case OW_BOILER_TEMP1:
                        boiler.hwater1 = temp;
                        break;
                    case OW_BOILER_TEMP2:
                        boiler.hwater2 = temp;
                        break;
                    case OW_FLOOR_OUT:
                        boiler.ht_floor_out = temp;
                        break;
                    case OW_FLOOR_RET:
                        boiler.ht_floor_ret = temp;
                        break;
                }
            }
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_CYCLE_CONTROL:
        {
            t = 0;
            res = stm_set_ow_ch(boiler.dev, 0);
            boiler_pid_floor();
            boiler_hot_water();
            break;
        }
        case ST_CYCLE_START ... ST_CYCLE_END:
        {
            t = 0;
            break;
        }
        default:
        {
            t = 0;
            s = 0;
            break;
        }
    }
    ++s;
    if(t)
    {
        send_mqtt(event, str);
    }

    boiler.tmr_value.it_value.tv_sec = 1;
    timerfd_settime(boiler.tmr, 0, &boiler.tmr_value, NULL);
}

void handle_boiler()
{
    uint64_t exp;
    int revents;

    revents = poll_fds.fds[boiler.n_fd].revents;
    if(revents)
    {
        poll_fds.fds[boiler.n_fd].revents = 0;
        read(boiler.tmr, &exp, sizeof(exp));
        tmrInterrupt0();
    }

}

void setup_boiler_poll()
{
    int n = poll_fds.n;
    boiler.n_fd = n;
    poll_fds.n += 1;
    
    poll_fds.fds[n].fd = boiler.tmr;
    poll_fds.fds[n].events = POLLIN;
    poll_fds.fds[n].revents = 0;
}

void init_boiler()
{
    int v, res;
    int dev;
    
    memset(&boiler, 0, sizeof(boiler));
    boiler.dev = -1;
    boiler.ds2482 = -1;
    boiler.tmr = -1;

    boiler.hwater1 = -200;
    boiler.hwater2 = -200;
    boiler.ht_floor_out = -200;
    boiler.ht_floor_ret = -200;

    boiler.pid1.p = cfg.boiler.pid1_p_gain;
    boiler.pid1.i = cfg.boiler.pid1_i_gain;

    boiler.ht_floor_set = 28;

    boiler.tmr_value.it_value.tv_sec = 5;
    boiler.tmr_value.it_value.tv_nsec = 0;
    boiler.tmr_value.it_interval.tv_sec = 0;
    boiler.tmr_value.it_interval.tv_nsec = 0;

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

    boiler.tmr = timerfd_create(CLOCK_MONOTONIC, 0);
    timerfd_settime(boiler.tmr, 0, &boiler.tmr_value, NULL);

    ha_register(&device, &en_pressure1);
    ha_register(&device, &en_pressure2);
    ha_register(&device, &en_t_boiler1);
    ha_register(&device, &en_t_boiler2);
    ha_register(&device, &en_t_boiler_in);
    ha_register(&device, &en_t_boiler_out);
    ha_register(&device, &en_t_boiler_ret);
    ha_register(&device, &en_t_floor_in);
    ha_register(&device, &en_t_floor_out);
    ha_register(&device, &en_t_floor_ret);
    ha_register(&device, &en_t_heating_out);
    ha_register(&device, &en_t_heating_ret);
    ha_register(&device, &en_t_gas_heater_in);
    ha_register(&device, &en_t_gas_heater_out);
    ha_register(&device, &en_t_gas_boiler_in);
    ha_register(&device, &en_t_gas_boiler_out);
    ha_register(&device, &en_t_ambient);
}

void close_boiler()
{
    if(boiler.dev > 0) close(boiler.dev);   
    boiler.dev = -1;

    if(boiler.ds2482 > 0) close(boiler.ds2482);
    boiler.ds2482 = -1;

    if(boiler.tmr > 0) close(boiler.tmr);
    boiler.tmr = -1;
}

void msg_boiler(int param, const char* message, size_t message_len)
{
    
}
