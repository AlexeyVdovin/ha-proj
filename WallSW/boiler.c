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

ha_entity_t en_pwm_1 =
{
    "boiler_pwm1",
    "sensor",
    "PWM 1",
    "power_factor",
    "%"
};

ha_entity_t en_pwm_2 =
{
    "boiler_pwm2",
    "sensor",
    "PWM 2",
    "power_factor",
    "%"
};

ha_entity_t en_water_heating =
{
    "boiler_hot_water_heating",
    "binary_sensor",
    "Water Heating",
    "power",
    NULL
};

ha_entity_t en_t_floor_set =
{
    "ht_floor_t_set",
    "number",
    "Heating Floor Temperature",
    NULL,
    NULL
};

ha_entity_t en_sw_hfloor_pump =
{
    "ht_floor_switch",
    "switch",
    "Heating Floor Pump",
    NULL,
    NULL
};

ha_entity_t en_sw_ch_pump =
{
    "ht_ch_switch",
    "switch",
    "Central Heating Pump",
    NULL,
    NULL
};

ha_entity_t en_sw_hw_pump =
{
    "ht_hwater_switch",
    "switch",
    "Hot Water Pump",
    NULL,
    NULL
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
    EE_ADR_RESERVED = 0,
    EE_ADR_FLOOR_T = 4
};

enum
{
    ST_INIT = -20,
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
    ST_CTL_CH6 = 0x20,
    ST_CTL_EEWR = 4096
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
    float   k;
    float   c;
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
// ------- Results --------
    float    pressure1;
    float    pressure2;
    float    boiler1;
    float    boiler2;
    float    boiler_in;
    float    boiler_out;
    float    boiler_ret;
    float    floor_in;
    float    floor_out;
    float    floor_ret;
    float    heating_out;
    float    heating_ret;
    float    gas_heater_in;
    float    gas_heater_out;
    float    gas_boiler_in;
    float    gas_boiler_out;
    float    ambient;
// ------- Settings --------
    float    floor_set;
    pd_t     pid1;
    struct itimerspec tmr_value;
} boil_t;

boil_t boiler;

void msg_boiler_t_hfloor(int param, const char* message, size_t message_len);
void msg_boiler_sw_hfloor(int param, const char* message, size_t message_len);
void msg_boiler_sw_ch_pump(int param, const char* message, size_t message_len);
void msg_boiler_sw_hw_pump(int param, const char* message, size_t message_len);
static int stm_set_control(int dev, uint16_t val);

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

static void eeprom_read_byte(uint8_t addr, uint8_t* value)
{
    union i2c_smbus_data data;
    int res, dev = boiler_i2c_open(0, 0x50+cfg.boiler.id);

    if(dev < 0) return;
    res = i2c_io(dev, I2C_SMBUS_READ, addr, I2C_SMBUS_BYTE_DATA, &data);
    if(res < 0)
    {
        close(dev);
        return;
    }
    *value = data.byte;
    close(dev);
    return;
}

static void eeprom_read_word(uint8_t addr, uint16_t* value)
{
    union i2c_smbus_data data;
    int res, dev = boiler_i2c_open(0, 0x50+cfg.boiler.id);

    if(dev < 0) return;
    res = i2c_io(dev, I2C_SMBUS_READ, addr, I2C_SMBUS_WORD_DATA, &data);
    if(res < 0)
    {
        close(dev);
        return;
    }
    *value = data.word;
    close(dev);
    return;
}

static void eeprom_write(uint8_t addr, uint8_t* value)
{
    union i2c_smbus_data data;
    uint16_t control = boiler.control;
    int res, dev = boiler_i2c_open(0, 0x50+cfg.boiler.id);

    if(dev < 0) return;
    control |= ST_CTL_EEWR;
    if(control != boiler.control)
    {
        stm_set_control(boiler.dev, control);
        boiler.control = control;
    }
    data.byte = *value;
    res = i2c_io(dev, I2C_SMBUS_WRITE, addr, I2C_SMBUS_BYTE_DATA, &data);
    usleep(10 * 1000); // Wait for write complete 5ms min
    control &= ~ST_CTL_EEWR;
    if(control != boiler.control)
    {
        stm_set_control(boiler.dev, control);
        boiler.control = control;
    }
    close(dev);
    return;
}

static inline void eeprom_write_byte(uint8_t addr, uint8_t* value)
{
    eeprom_write(addr, value);
    return;
}

static inline void eeprom_write_word(uint8_t addr, uint16_t* value)
{
    uint8_t byte = (uint8_t)((*value) & 0x00FF);
    eeprom_write(addr, &byte);
    byte = (uint8_t)(((*value) >> 8) & 0x00FF);
    eeprom_write(addr+1, &byte);
    return;
}

static int boiler_set_pwm1(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_PWM1, I2C_SMBUS_WORD_DATA, &data);
}

static int boiler_get_pwm1(int dev, uint16_t *val)
{
    int res;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, STM_PWM1, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;

    return res;
}

static int stm_set_control(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_CONTROL, I2C_SMBUS_WORD_DATA, &data);
}

static int stm_get_control(int dev, uint16_t *val)
{
    int res;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, STM_CONTROL, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;

    return res;
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
  float delta = (p->t > 0) ? p->t - t : 0;
  int32_t out = (int32_t)(p->p * error / 1024);
  out += (int32_t)(p->d * delta / 1024);
  p->integral += p->i * error / 1024;
  if(p->integral > 1024) p->integral = 1024;
  if(p->integral < -1023) p->integral = -1023;
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

    if(boiler.boiler1 != -200) t = boiler.boiler1;
    if(boiler.boiler2 != -200) t = boiler.boiler2;

    DBG("Hot Water: T = %.1f, state = %s", t, state ? "On" : "Off");

    if(t < cfg.boiler.hwater_min) state = 1;
    if(t > cfg.boiler.hwater_max) state = 0;

    control &= ~ST_CTL_CH6;
    control |= state ? ST_CTL_CH6 : 0;
    if(control != boiler.control)
    {
        stm_set_control(boiler.dev, control);
        boiler.control = control;
        mqtt_send_status(&en_water_heating, state ? "ON" : "OFF");
    }
}

static void boiler_pid_floor()
{
    float t;
    int32_t out = 0;
    uint16_t pwm = 0;
    char status[20];

    if(boiler.floor_out != -200 && boiler.floor_ret != -200)
    {
        t = (boiler.floor_out + boiler.floor_ret)/2;
        out = update_pid(&boiler.pid1, boiler.floor_set, t);
        pwm = (out > cfg.boiler.pwm1_max) ? cfg.boiler.pwm1_max : (out < cfg.boiler.pwm1_min) ? cfg.boiler.pwm1_min : out;
        boiler_set_pwm1(boiler.dev, pwm);
        
//        snprintf(status, sizeof(status)-1, "%.1f", boiler.floor_set);
//        mqtt_send_status(&en_t_floor_set, status);

        snprintf(status, sizeof(status)-1, "%.1f", pwm/10.0f);
        mqtt_send_status(&en_pwm_1, status);
    }
}

static void boiler_ch_setup(uint16_t ch, int *n, ow_t* ow)
{
    int i;
    uint16_t val = 0;
    char str[32];

    boiler_ch_enum(ch, &val);
    *n = val;
    boiler_ch_measure();
    for(i = 0; i < val; ++i)
    {
        int x;
        uint8_t* addr = ow[i].addr;
        ow[i].type = OW_UNKNOWN;
        sprintf(str, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
        for(x = 0; x < OW_UNKNOWN; ++x)
        {
            if(memcmp(str, cfg.boiler.sensor[x], strlen(str)) == 0)
            {
                ow[i].type = x;
                ow[i].k = cfg.boiler.sensor_k[x];
                ow[i].c = cfg.boiler.sensor_c[x];
                break;
            }
        }
    }
}

static void boiler_ch_results(uint16_t ch, int n, ow_t* ow)
{
    int res, i;
    int16_t value;
    char str[64], status[20];

    for(i = 0; i < n; ++i)
    {
        ow[i].t = -200;
        res = boiler_ch_read(ow[i].addr, &value);
        if(res == 0)
        {
            ow[i].t = (value/16.0f+ow[i].c)*ow[i].k;
            // DBG("ch%d: %d = (%f + %f) * %f", ch, i, value/16.0f, ow[i].c, ow[i].k);
            snprintf(status, sizeof(status)-1, "%.2f", ow[i].t);
            switch(ow[i].type)
            {
                case OW_BOILER_TEMP1:
                    if(boiler.boiler1 != ow[i].t)
                    {
                        boiler.boiler1 = ow[i].t;
                        mqtt_send_status(&en_t_boiler1, status);
                    }
                    break;
                case OW_BOILER_TEMP2:
                    if(boiler.boiler2 != ow[i].t)
                    {
                        boiler.boiler2 = ow[i].t;
                        mqtt_send_status(&en_t_boiler2, status);
                    }
                    break;
                case OW_BOILER_IN:
                    if(boiler.boiler_in != ow[i].t)
                    {
                        boiler.boiler_in = ow[i].t;
                        mqtt_send_status(&en_t_boiler_in, status);
                    }
                    break;
                case OW_BOILER_OUT:
                    if(boiler.boiler_out != ow[i].t)
                    {
                        boiler.boiler_out = ow[i].t;
                        mqtt_send_status(&en_t_boiler_out, status);
                    }
                    break;
                case OW_BOILER_RET:
                    if(boiler.boiler_ret != ow[i].t)
                    {
                        boiler.boiler_ret = ow[i].t;
                        mqtt_send_status(&en_t_boiler_ret, status);
                    }
                    break;
                case OW_FLOOR_IN:
                    if(boiler.floor_in != ow[i].t)
                    {
                        boiler.floor_in = ow[i].t;
                        mqtt_send_status(&en_t_floor_in, status);
                    }
                    break;
                case OW_FLOOR_OUT:
                    if(boiler.floor_out != ow[i].t)
                    {
                        boiler.floor_out = ow[i].t;
                        mqtt_send_status(&en_t_floor_out, status);
                    }
                    break;
                case OW_FLOOR_RET:
                    if(boiler.floor_ret != ow[i].t)
                    {
                        boiler.floor_ret = ow[i].t;
                        mqtt_send_status(&en_t_floor_ret, status);
                    }
                    break;
                case OW_PIPE_OUT:
                    if(boiler.heating_out != ow[i].t)
                    {
                        boiler.heating_out = ow[i].t;
                        mqtt_send_status(&en_t_heating_out, status);
                    }
                    break;
                case OW_PIPE_RET:
                    if(boiler.heating_ret != ow[i].t)
                    {
                        boiler.heating_ret = ow[i].t;
                        mqtt_send_status(&en_t_heating_ret, status);
                    }
                    break;
                case OW_HEAT_IN:
                    if(boiler.gas_heater_in != ow[i].t)
                    {
                        boiler.gas_heater_in = ow[i].t;
                        mqtt_send_status(&en_t_gas_heater_in, status);
                    }
                    break;
                case OW_HEAT_OUT:
                    if(boiler.gas_heater_out != ow[i].t)
                    {
                        boiler.gas_heater_out = ow[i].t;
                        mqtt_send_status(&en_t_gas_heater_out, status);
                    }
                    break;
                case OW_HWATER_IN:
                    if(boiler.gas_boiler_in != ow[i].t)
                    {
                        boiler.gas_boiler_in = ow[i].t;
                        mqtt_send_status(&en_t_gas_boiler_in, status);
                    }
                    break;
                case OW_HWATER_OUT:
                    if(boiler.gas_boiler_out != ow[i].t)
                    {
                        boiler.gas_boiler_out = ow[i].t;
                        mqtt_send_status(&en_t_gas_boiler_out, status);
                    }
                    break;
                case OW_AMBIENT:
                    if(boiler.ambient != ow[i].t)
                    {
                        boiler.ambient = ow[i].t;
                        mqtt_send_status(&en_t_ambient, status);
                    }
                    break;
                case OW_UNKNOWN:
                    snprintf(str, sizeof(str)-1, "temperature.%02x%02x%02x%02x%02x%02x%02x%02x", ow[i].addr[0], ow[i].addr[1], ow[i].addr[2], ow[i].addr[3], ow[i].addr[4], ow[i].addr[5], ow[i].addr[6], ow[i].addr[7]);
                    send_mqtt(str, status);
                    break;
            }
        }
    }
}

static void tmrInterrupt0(void)
{
    static int s = ST_INIT;
    uint16_t val;
    int16_t value;
    const char* event;
    char str[32];
    int res, i, t = time(0);
    float f;

   // DBG("boiler: Tmr INT: %d", s);

    switch(s)
    {
        case ST_INIT:
        {
            snprintf(str, sizeof(str)-1, "%.1f", boiler.floor_set);
            mqtt_send_status(&en_t_floor_set, str);
            mqtt_send_status(&en_sw_hfloor_pump, (boiler.control & ST_CTL_CH3) ? "ON" : "OFF");
            mqtt_send_status(&en_sw_ch_pump, (boiler.control & ST_CTL_CH4) ? "ON" : "OFF");
            mqtt_send_status(&en_sw_hw_pump, (boiler.control & ST_CTL_CH5) ? "ON" : "OFF");
            mqtt_send_status(&en_water_heating, (boiler.control & ST_CTL_CH6) ? "ON" : "OFF");
        }
        case ST_DC_12V:
        {
            event = STR_DC_12V;
            stm_get_dc(boiler.dev, STM_DC_12V, &val);
            break;
        }
        case ST_ADC_IN1:
        {
            event = STR_ADC_IN1;
            stm_get_dc(boiler.dev, STM_ADC_IN1, &val);
            f = (val-1200)/790.0f;
            if(boiler.pressure1 != f)
            {
                boiler.pressure1 = f;
                sprintf(str, "%.1f", f);
                mqtt_send_status(&en_pressure1, str);
            }
            break;
        }
        case ST_ADC_IN2:
        {
            event = STR_ADC_IN2;
            stm_get_dc(boiler.dev, STM_ADC_IN2, &val);
            f = (val-1200)/790.0f;
            if(boiler.pressure2 != f)
            {
                boiler.pressure2 = f;
                sprintf(str, "%.1f", f);
                mqtt_send_status(&en_pressure2, str);
            }
            break;
        }
        case ST_IN_VDD:
        {
            event = STR_VDD_OK;
            stm_get_vdd(boiler.dev, &val);
            break;
        }
        case ST_OW_CH1_ENUM:
        {
            event = STR_OW_CH1_ENUM;
            boiler_ch_setup(1, &boiler.ch1_n, boiler.ch1);
            break;
        }
        case ST_OW_CH1_READ:
        {
            boiler_ch_results(1, boiler.ch1_n, boiler.ch1);
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_OW_CH2_ENUM:
        {
            event = STR_OW_CH2_ENUM;
            boiler_ch_setup(2, &boiler.ch2_n, boiler.ch2);
            break;
        }
        case ST_OW_CH2_READ:
        {
            boiler_ch_results(2, boiler.ch2_n, boiler.ch2);
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_OW_CH3_ENUM:
        {
            event = STR_OW_CH3_ENUM;
            boiler_ch_setup(3, &boiler.ch3_n, boiler.ch3);
            break;
        }
        case ST_OW_CH3_READ:
        {
            boiler_ch_results(3, boiler.ch3_n, boiler.ch3);
            res = stm_set_ow_ch(boiler.dev, 0);
            break;
        }
        case ST_OW_CH4_ENUM:
        {
            event = STR_OW_CH4_ENUM;
            boiler_ch_setup(4, &boiler.ch4_n, boiler.ch4);
            break;
        }
        case ST_OW_CH4_READ:
        {
            boiler_ch_results(4, boiler.ch4_n, boiler.ch4);
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
    uint16_t t;
    
    memset(&boiler, 0, sizeof(boiler));
    boiler.dev = -1;
    boiler.ds2482 = -1;
    boiler.tmr = -1;

    boiler.pressure1 = -1;
    boiler.pressure2 = -1;
    boiler.boiler1 = -200;
    boiler.boiler2 = -200;
    boiler.boiler_in = -200;
    boiler.boiler_out = -200;
    boiler.boiler_ret = -200;
    boiler.floor_in = -200;
    boiler.floor_out = -200;
    boiler.floor_ret = -200;
    boiler.heating_out = -200;
    boiler.heating_ret = -200;
    boiler.gas_heater_in = -200;
    boiler.gas_heater_out = -200;
    boiler.gas_boiler_in = -200;
    boiler.gas_boiler_out = -200;
    boiler.ambient = -200;

    boiler.pid1.p = cfg.boiler.pid1_p_gain;
    boiler.pid1.i = cfg.boiler.pid1_i_gain;

    boiler.floor_set = 23;

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

    res = stm_set_ow_ch(dev, boiler.ow_ch);

    res = boiler_get_pwm1(boiler.dev, &t);
    if(res >= 0)
    {
        boiler.pid1.integral = t;
    }

    res = stm_get_control(boiler.dev, &t);
    if(res >= 0)
    {
        boiler.control = t;
    }

    t = 23 * 16;
    eeprom_read_word(EE_ADR_FLOOR_T, &t);
    boiler.floor_set = t / 16.0f;

    dev = ds2482_open(0, 0x18 + cfg.boiler.id);
    if(dev < 0)
    {
        DBG("Error: boiler_i2c_open(0x%02x) failed %d!", 0x18 + cfg.boiler.id, errno);
        return;
    }
    boiler.ds2482 = dev;

    boiler.tmr = timerfd_create(CLOCK_MONOTONIC, 0);
    timerfd_settime(boiler.tmr, 0, &boiler.tmr_value, NULL);

    ha_register_sensor(&device, &en_pressure1);
    ha_register_sensor(&device, &en_pressure2);
    ha_register_sensor(&device, &en_t_boiler1);
    ha_register_sensor(&device, &en_t_boiler2);
    ha_register_sensor(&device, &en_t_boiler_in);
    ha_register_sensor(&device, &en_t_boiler_out);
    ha_register_sensor(&device, &en_t_boiler_ret);
    ha_register_sensor(&device, &en_t_floor_in);
    ha_register_sensor(&device, &en_t_floor_out);
    ha_register_sensor(&device, &en_t_floor_ret);
    ha_register_sensor(&device, &en_t_heating_out);
    ha_register_sensor(&device, &en_t_heating_ret);
    ha_register_sensor(&device, &en_t_gas_heater_in);
    ha_register_sensor(&device, &en_t_gas_heater_out);
    ha_register_sensor(&device, &en_t_gas_boiler_in);
    ha_register_sensor(&device, &en_t_gas_boiler_out);
    ha_register_sensor(&device, &en_t_ambient);
    ha_register_sensor(&device, &en_pwm_1);
    ha_register_sensor(&device, &en_pwm_2);
    ha_register_binary(&device, &en_water_heating);
    ha_register_number(&device, &en_t_floor_set);
    ha_register_switch(&device, &en_sw_hfloor_pump);
    ha_register_switch(&device, &en_sw_ch_pump);
    ha_register_switch(&device, &en_sw_hw_pump);
    
    set_uplink_filter(en_t_floor_set.unique_id, msg_boiler_t_hfloor, 0);
    set_uplink_filter(en_sw_hfloor_pump.unique_id, msg_boiler_sw_hfloor, 0);
    set_uplink_filter(en_sw_ch_pump.unique_id, msg_boiler_sw_ch_pump, 0);
    set_uplink_filter(en_sw_hw_pump.unique_id, msg_boiler_sw_hw_pump, 0);
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

void msg_boiler_t_hfloor(int param, const char* message, size_t message_len)
{
    int n = 0;
    float v;
    
    n = sscanf(message, "%f", &v);
    if(n == 1)
    {
        DBG("boiler.t_floor_set: %.1f", v);
        if(boiler.floor_set != v)
        {
            boiler.floor_set = v;
            uint16_t t = (uint16_t)(v * 16);
            eeprom_write_word(EE_ADR_FLOOR_T, &t);
        }
        return;
    }
}

void msg_boiler_sw_hfloor(int param, const char* message, size_t message_len)
{
    int n = 0;
    uint16_t control = boiler.control;
    uint16_t state = (control & ST_CTL_CH3);

    if(message_len == 2 && memcmp(message, "ON", message_len) == 0)
    {
        state = 1;
    }
    else if(message_len == 3 && memcmp(message, "OFF", message_len) == 0)
    {
        state = 0;
    }

    control &= ~ST_CTL_CH3;
    control |= state ? ST_CTL_CH3 : 0;
    stm_set_control(boiler.dev, control);
    boiler.control = control;
    mqtt_send_status(&en_sw_hfloor_pump, state ? "ON" : "OFF");
}

void msg_boiler_sw_ch_pump(int param, const char* message, size_t message_len)
{
    int n = 0;
    uint16_t control = boiler.control;
    uint16_t state = (control & ST_CTL_CH4);

    if(message_len == 2 && memcmp(message, "ON", message_len) == 0)
    {
        state = 1;
    }
    else if(message_len == 3 && memcmp(message, "OFF", message_len) == 0)
    {
        state = 0;
    }

    control &= ~ST_CTL_CH4;
    control |= state ? ST_CTL_CH4 : 0;
    stm_set_control(boiler.dev, control);
    boiler.control = control;
    mqtt_send_status(&en_sw_ch_pump, state ? "ON" : "OFF");
}

void msg_boiler_sw_hw_pump(int param, const char* message, size_t message_len)
{
    int n = 0;
    uint16_t control = boiler.control;
    uint16_t state = (control & ST_CTL_CH5);

    if(message_len == 2 && memcmp(message, "ON", message_len) == 0)
    {
        state = 1;
    }
    else if(message_len == 3 && memcmp(message, "OFF", message_len) == 0)
    {
        state = 0;
    }

    control &= ~ST_CTL_CH5;
    control |= state ? ST_CTL_CH5 : 0;
    stm_set_control(boiler.dev, control);
    boiler.control = control;
    mqtt_send_status(&en_sw_hw_pump, state ? "ON" : "OFF");
}
