#define _XOPEN_SOURCE

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <libmemcached/memcached.h>

#include "minini/minIni.h"
#include "config.h"
#include "uplink.h"
#include "ds2482.h"
#include "pca9554.h"
#include "wiringPi/wiringPi.h"
#include "stm.h"

enum
{
    ST_IDLE = 0,
    ST_DC_12V,
    ST_DC_BATT,
    ST_DC_5V0,
    ST_DC_3V3,
    ST_DS_ENUM1,
    ST_DS_READ1,
    ST_DS_ENUM2,
    ST_DS_READ2,
    ST_DS_STAT,
    ST_WATERING
};

static const char STR_DC_12V[]      = "dc:12V";
static const char STR_DC_BAT[]      = "dc:BATT";
static const char STR_DC_5V0[]      = "dc:5V0";
static const char STR_DC_3V3[]      = "dc:3V3";
static const char STR_DC_VCC[]      = "dc:VCC";
static const char STR_DC_REF[]      = "dc:REF";
static const char STR_TEMPERATURE[] = "TEMPERATURE";

enum
{
    DS_G1_AIR = 0,
    DS_G1_GROUND,
    DS_G2_AIR,
    DS_G2_GROUND
};

enum
{
  STM_CTL_LOW_PWR   = 0x0001,
  STM_CTL_DR1       = 0x0002,
  STM_CTL_DR2       = 0x0004,
  STM_CTL_ENABLE_PM = 0x8000
};

typedef struct
{
    int16_t t;
    time_t  time;
} min_t;

typedef struct
{
    int      n_ints;
    int      dev;
    int      n_fd;
    int      state;
    uint16_t control;
    uint16_t adc_12v;
    uint16_t adc_batt;
    uint16_t adc_z5v0;
    uint16_t adc_z3v3;
    min_t    t_min[8];
    min_t    t_max[8];
    int16_t  t_avg[8];
    int16_t  temperature[8];
    int16_t  arr_tmp[8][8];
    uint8_t  arr_n[8];
    uint8_t  port[8];
    uint8_t  type[8];
    uint8_t  addr[8][8];
    uint8_t  ds_n;
    uint8_t  G1_vent;
    uint8_t  G1_heat;
    uint8_t  G1_circ;
    uint8_t  G1_water;
    uint8_t  G2_vent;
    uint8_t  G2_heat;
    uint8_t  G2_circ;
    uint8_t  G2_water;
    time_t G1_watering_last;
    time_t G2_watering_last;
    memcached_st mc;
} stm_t;

stm_t stm;

static int stm_open(uint8_t bus, uint8_t adr)
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

static int stm_set_control(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_CONTROL, I2C_SMBUS_WORD_DATA, &data);
}

static int stm_get_status(int dev, uint16_t *val)
{
    int res;
    union i2c_smbus_data data;
    
    res = i2c_io(dev, I2C_SMBUS_READ, STM_STATUS, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val |= data.word;

    return res;
}

static int stm_get_dc(int dev, uint8_t in, uint16_t *val)
{
    int res;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, in, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;
    
    return res;
}

// stm32 Interrupt PA07 pin
static void gpioInterrupt1(void) 
{ 
    stm.n_ints++;
    DBG("Interrupt: stm32");
}

static int stm_find_sensor(uint8_t addr[8])
{
    char id[32];
    int res = -1;
    
    sprintf(id, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);

    if(memcmp(id, cfg.G1_air, 16) == 0) res = DS_G1_AIR;
    else if(memcmp(id, cfg.G1_ground, 16) == 0) res = DS_G1_GROUND;
    else if(memcmp(id, cfg.G2_air, 16) == 0) res = DS_G2_AIR;
    else if(memcmp(id, cfg.G2_ground, 16) == 0) res = DS_G2_GROUND;

    return res;
}

static int stm_ds2482_enum(uint8_t port)
{
    int res, i, n = stm.ds_n;
    uint8_t status;
    ow_serch_t search;

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

        // DBG("Found 1W device [%d]: 0x%02x%02x%02x%02x%02x%02x%02x%02x", n, search.addr[0], search.addr[1], search.addr[2], search.addr[3], search.addr[4], search.addr[5], search.addr[6], search.addr[7]);
        memcpy(stm.addr[n], search.addr, sizeof(stm.addr[0]));
        stm.port[n] = port;
        ++n;
    } while(1);

    if(n == 0)
    {
        DBG("Error: No 1W devices found!\n");
        return res;
    }
    stm.ds_n = n;

    do
    {
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

    } while(0);

}

static int stm_ds2482_read(int port)
{
    int res, i, n = stm.ds_n;
    uint8_t status;
    uint8_t data[9];

    for(i = 0; i < n; ++i)
    {
        if(stm.port[i] != port) continue;

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

        res = ds2482_1w_match(stm.addr[i]);
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
        // DBG("Read temperature: 0x%02x 0x%02x, %.4f", data[0], data[1], t/16.0);
        DBG("0x%02x%02x%02x%02x%02x%02x%02x%02x %d", stm.addr[i][0], stm.addr[i][1], stm.addr[i][2], stm.addr[i][3], stm.addr[i][4], stm.addr[i][5], stm.addr[i][6], stm.addr[i][7], (int)(t*125/2));
        stm.temperature[i] = t*125/2;
    }
    return res;
}

static void stm_mc_sets(const char* key, const char* val)
{
    memcached_return rc;

    DBG("Memcached set: %s => %s", key, val);
    rc = memcached_set(&stm.mc, key, strlen(key), val, strlen(val), 0, 0);
    if(rc != MEMCACHED_SUCCESS)
    {
        DBG("Error: Set memcached value failed %s => %s", key, val);
    }
}

static void stm_mc_setn(const char* key, int val)
{
    memcached_return rc;
    char str[16];

    sprintf(str, "%d", val);
    DBG("Memcached set: %s => %s", key, str);
    rc = memcached_set(&stm.mc, key, strlen(key), str, strlen(str), 0, 0);
    if(rc != MEMCACHED_SUCCESS)
    {
        DBG("Error: Set memcached value failed %s => %s", key, str);
    }
}

static void stm_mc_setf(const char* key, float val)
{
    memcached_return rc;
    char str[16];

    sprintf(str, "%.2f", val);
    DBG("Memcached set: %s => %s", key, str);
    rc = memcached_set(&stm.mc, key, strlen(key), str, strlen(str), 0, 0);
    if(rc != MEMCACHED_SUCCESS)
    {
        DBG("Error: Set memcached value failed %s => %s", key, str);
    }
}

static void stm_mc_setk(const char* pre, uint8_t* id, int val)
{
    memcached_return rc;
    char str[16], key[64];

    sprintf(str, "%d", val);
    sprintf(key, "%s%02x%02x%02x%02x%02x%02x%02x%02x", pre, id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
    DBG("Memcached set: %s => %s", key, str);
    rc = memcached_set(&stm.mc, key, strlen(key), str, strlen(str), 0, 0);
    if(rc != MEMCACHED_SUCCESS)
    {
        DBG("Error: Set memcached value failed %s => %s", key, str);
    }
}

static int stm_ds2482_pub(int port)
{
    int i, n = stm.ds_n;
    char key[32];

    for(i = 0; i < n; ++i)
    {
        if(stm.port[i] != port) continue;
        stm_mc_setk("t_", stm.addr[i], stm.temperature[i]);
    }
    return 0;
}

static void stm_ds_stat()
{
    memcached_return rc;
    int x, c, s, i, n = stm.ds_n;
    float g1a = -55, g1g = -55, g2a = -55, g2g = -55;
    float sum;

    for(i = 0; i < n; ++i)
    {
        c = stm.arr_n[i];
        if(c < cfg.n_avg) stm.arr_tmp[i][c] = stm.temperature[i];
        else
        {
            c = cfg.n_avg-1;
            for(x = 1; x < c; ++x)
            {
                stm.arr_tmp[i][x-1] = stm.arr_tmp[i][x];
            }
            stm.arr_tmp[i][c] = stm.temperature[i];
        }
        // DBG("Stat: [%d] %d %d", i, c, stm.temperature[i]);
        stm.arr_n[i] = ++c;

        sum = 0;
        for(x = 0; x <= c; ++x)
        {
            sum += stm.arr_tmp[i][x];
        }
        stm.t_avg[i] = sum/c;
        stm_mc_setk("", stm.addr[i], stm.t_avg[i]);

        if(stm.t_min[i].t > stm.temperature[i]) { stm.t_min[i].t = stm.temperature[i]; stm.t_min[i].time = time(0); stm_mc_setk("min_", stm.addr[i], stm.t_min[i].t); }
        if(stm.t_max[i].t < stm.temperature[i]) { stm.t_max[i].t = stm.temperature[i]; stm.t_max[i].time = time(0); stm_mc_setk("max_", stm.addr[i], stm.t_max[i].t); }

        s = stm_find_sensor(stm.addr[i]);
        if(s != -1)
        {
            // DBG("Sum [%d] %f", s, sum);
            switch(s)
            {
                case DS_G1_AIR:
                    g1a = stm.t_avg[i]/1000.0;
                    DBG("[%d] G1 Air: %.2f (%.2f)", i, stm.temperature[i]/1000.0, g1a);
                    break;
                case DS_G1_GROUND:
                    g1g = stm.t_avg[i]/1000.0;
                    DBG("[%d] G1 Ground: %.2f (%.2f)", i, stm.temperature[i]/1000.0, g1g);
                    break;
                case DS_G2_AIR:
                    g2a = stm.t_avg[i]/1000.0;
                    DBG("[%d] G2 Air: %.2f (%.2f)", i, stm.temperature[i]/1000.0, g2a);
                    break;
                case DS_G2_GROUND:
                    g2g = stm.t_avg[i]/1000.0;
                    DBG("[%d] G2 Ground: %.2f (%.2f)", i, stm.temperature[i]/1000.0, g2g);
                    break;
                default:
                    break;
            }
        }
    }

    if(g1a != -55)
    {
        if(cfg.G1_set.heat == 2)
        {
            if(stm.G1_heat == 0)
            {
                if(g1a < cfg.heating) stm.G1_heat = 1;
            }
            else
            {
                if(g1a > cfg.heating+1) stm.G1_heat = 0;
            }
        }
        if(cfg.G1_set.vent == 2)
        {
            if(stm.G1_vent == 0)
            {
                if(g1a > cfg.ventilation) stm.G1_vent = 1;
            }
            else
            {
                if(g1a < cfg.ventilation-3) stm.G1_vent = 0;
            }
        }
        stm_mc_setn("G1_HEAT", stm.G1_heat);
        stm_mc_setn("G1_VENT", stm.G1_vent);
    }
    if(g1a != -55 && g1g != -55)
    {
        if(cfg.G1_set.circ == 2)
        {
            if(stm.G1_circ == 0)
            {
                if(g1a - g1g > cfg.circ_delta+2 && g1g < 18) stm.G1_circ = 1;
            }
            else
            {
                if(g1a - g1g < 2 || g1g > 21) stm.G1_circ = 0;
            }
        }
        stm_mc_setn("G1_CIRC", stm.G1_circ);
    }
    if(g2a != -55)
    {
        if(cfg.G2_set.heat == 2)
        {
            if(stm.G2_heat == 0)
            {
                if(g2a < cfg.heating) stm.G2_heat = 1;
            }
            else
            {
                if(g2a > cfg.heating+1) stm.G2_heat = 0;
            }
        }
        if(cfg.G2_set.vent == 2)
        {
            if(stm.G2_vent == 0)
            {
                if(g2a > cfg.ventilation) stm.G2_vent = 1;
            }
            else
            {
                if(g2a < cfg.ventilation-3) stm.G2_vent = 0;
            }
        }        
        stm_mc_setn("G2_HEAT", stm.G2_heat);
        stm_mc_setn("G2_VENT", stm.G2_vent);
    }
    if(g2a != -55 && g2g != -55)
    {
        if(cfg.G2_set.circ == 2)
        {
            if(stm.G2_circ == 0)
            {
                if(g2a - g2g > cfg.circ_delta+2 && g2g < 18) stm.G2_circ = 1;
            }
            else
            {
                if(g2a - g2g < 2 || g2g > 21) stm.G2_circ = 0;
            }
        }
        stm_mc_setn("G2_CIRC", stm.G2_circ);
    }
    
    set_pca9554(PCA9554_OUT_X2, stm.G1_vent + stm.G2_vent);
    set_pca9554(PCA9554_OUT_X11, stm.G1_heat + stm.G2_heat);
    set_pca9554(PCA9554_OUT_X6, stm.G1_circ);
    set_pca9554(PCA9554_OUT_X8, stm.G2_circ);
}

static int read_settings()
{
    static int mtime = 0;
    int res;
    struct stat st;
    const char* name = cfg.settings;

    res = stat(name, &st);
    if(res)
    {
        DBG("Error: Stat(%s) error: %d", name, errno);
        return res;
    }
    if(mtime != st.st_mtime)
    {
        mtime = st.st_mtime;
        cfg.G1_set.vent     = ini_getl("general", "G1_VENT_C", 2, name); if(cfg.G1_set.vent < 0 || cfg.G1_set.vent > 2) cfg.G1_set.vent = 2;
        cfg.G1_set.heat     = ini_getl("general", "G1_HEAT_C", 2, name); if(cfg.G1_set.heat < 0 || cfg.G1_set.heat > 2) cfg.G1_set.heat = 2;
        cfg.G1_set.circ     = ini_getl("general", "G1_CIRC_C", 2, name); if(cfg.G1_set.circ < 0 || cfg.G1_set.circ > 2) cfg.G1_set.circ = 2;
        cfg.G1_set.water    = ini_getl("general", "G1_WATER_C", 2, name); if(cfg.G1_set.water < 0 || cfg.G1_set.water > 2) cfg.G1_set.water = 2;
        cfg.G1_set.period   = ini_getl("general", "G1_PERIOD", 180, name); if(cfg.G1_set.period < 10 || cfg.G1_set.period > 1440) cfg.G1_set.period = 180;
        cfg.G1_set.duration = ini_getl("general", "G1_DURATION", 2, name); if(cfg.G1_set.duration < 0 || cfg.G1_set.duration > 60) cfg.G1_set.duration = 2;

        cfg.G2_set.vent     = ini_getl("general", "G2_VENT_C", 2, name); if(cfg.G2_set.vent < 0 || cfg.G2_set.vent > 2) cfg.G2_set.vent = 2;
        cfg.G2_set.heat     = ini_getl("general", "G2_HEAT_C", 2, name); if(cfg.G2_set.heat < 0 || cfg.G2_set.heat > 2) cfg.G2_set.heat = 2;
        cfg.G2_set.circ     = ini_getl("general", "G2_CIRC_C", 2, name); if(cfg.G2_set.circ < 0 || cfg.G2_set.circ > 2) cfg.G2_set.circ = 2;
        cfg.G2_set.water    = ini_getl("general", "G2_WATER_C", 2, name); if(cfg.G2_set.water < 0 || cfg.G2_set.water > 2) cfg.G2_set.water = 2;
        cfg.G2_set.period   = ini_getl("general", "G2_PERIOD", 180, name); if(cfg.G2_set.period < 10 || cfg.G2_set.period > 1440) cfg.G2_set.period = 180;
        cfg.G2_set.duration = ini_getl("general", "G2_DURATION", 2, name); if(cfg.G2_set.duration < 0 || cfg.G2_set.duration > 60) cfg.G2_set.duration = 2;
        res = 1;
    }
    // <0 - error
    //  0 - no changes
    // >0 - changed
    return res;
}

static void stm_save_time()
{
    FILE* fl = fopen(cfg.watering, "w");
    struct tm tm;
    char str[128];

    if(fl != NULL)
    {
        localtime_r(&stm.G1_watering_last, &tm);
        strftime(str, sizeof(str), "%Y-%m-%d %H:%M", &tm);
        fprintf(fl, "%s\n", str);
        DBG("Save G1 last watering time: %s", str);

        localtime_r(&stm.G2_watering_last, &tm);
        strftime(str, sizeof(str), "%Y-%m-%d %H:%M", &tm);
        fprintf(fl, "%s\n", str);
        DBG("Save G2 last watering time: %s", str);

        fflush(fl);
        fclose(fl);
    }
}

static void stm_get_time()
{
    struct tm tm;
    char str[128];
    FILE* fl = fopen(cfg.watering, "r");

    while(fl != NULL)
    {
        // strptime("2022-01-12 10:30", "%Y-%m-%d %H:%M", &tm);
        if(fgets(str, sizeof(str), fl) == NULL)
        {
            DBG("Error: Reading last watering time %d", errno);
            break;
        }

        memset(&tm, 0, sizeof(tm));
        if(strptime(str, "%Y-%m-%d %H:%M", &tm) == NULL)
        {
            DBG("Error: Parcing last watering time!");
            break;
        }
        stm.G1_watering_last = mktime(&tm);
        strftime(str, sizeof(str), "%Y-%m-%d %H:%M", &tm);
        DBG("G1 last watering time: %s", str);

        if(fgets(str, sizeof(str), fl) == NULL)
        {
            DBG("Error: Reading last watering time %d", errno);
            break;
        }

        memset(&tm, 0, sizeof(tm));
        if(strptime(str, "%Y-%m-%d %H:%M", &tm) == NULL)
        {
            DBG("Error: Parcing last watering time!");
            break;
        }
        stm.G2_watering_last = mktime(&tm);
        strftime(str, sizeof(str), "%Y-%m-%d %H:%M", &tm);
        DBG("G2 last watering time: %s", str);

        break;
    }
    if(fl != 0) fclose(fl);
}

static void stm_G1_watering(int on)
{
    stm.control =  (stm.control & (~STM_CTL_DR1)) | (on ? STM_CTL_DR1 : 0);
    stm_set_control(stm.dev, stm.control);
}

static void stm_G2_watering(int on)
{
    stm.control =  (stm.control & (~STM_CTL_DR2)) | (on ? STM_CTL_DR2 : 0);
    stm_set_control(stm.dev, stm.control);
}

static void stm_watering()
{
    static int g1w = 0, g2w = 0;
    time_t t = time(0);
    int save = 0;

    if(g1w == 0)
    {
        // DBG("DBG: Watering last: %d, period: %d, time: %d ? %d", (int)stm.G1_watering_last, (int)cfg.G1_set.period, (int)(stm.G1_watering_last + cfg.G1_set.period * 60), (int)t);
        if((stm.G1_watering_last + cfg.G1_set.period * 60 < t  && stm.G1_water == 2) || stm.G1_water == 1)
        {
            g1w = 1;
            stm_G1_watering(1);
            stm_mc_setn("G1_WATER", 1);
            if(stm.G1_water == 2)
            {
                stm.G1_watering_last = t;
                save = 1;
            }
        }
    }
    else
    {
        if((stm.G1_watering_last + cfg.G1_set.duration * 60 < t && stm.G1_water == 2) || stm.G1_water == 0)
        {
            g1w = 0;
            stm_G1_watering(0);
            stm_mc_setn("G1_WATER", 0);
        }
    }

    if(g2w == 0)
    {
        if((stm.G2_watering_last + cfg.G2_set.period * 60 < t && stm.G2_water == 2) || stm.G2_water == 1)
        {
            g2w = 1;
            stm_G2_watering(1);
            stm_mc_setn("G2_WATER", 1);
            if(stm.G2_water == 2)
            {
                stm.G2_watering_last = t;
                save = 1;
            }
        }
    }
    else
    {
        if((stm.G2_watering_last + cfg.G2_set.duration * 60 < t && stm.G2_water == 2) || stm.G2_water == 0)
        {
            g2w = 0;
            stm_G2_watering(0);
            stm_mc_setn("G2_WATER", 0);
        }
    }

    if(save)
    {
        stm_save_time();
    }
}

void handle_stm()
{
    static int m = 0, s = -20;
    int t, upd;
    uint8_t c;
    uint16_t val;
    int save = 0;
    int revents = poll_fds.fds[stm.n_fd].revents;
    
    if(revents)
    {
        poll_fds.fds[stm.n_fd].revents = 0;
        gpioInterrupt1();
        read(getInterruptFS(PIN_PA7), &c, 1);
    }

    t = time(0);
    if(t != s)
    {
        s = t;
        DBG("stm: %d", m);
        upd = read_settings();
        if(upd > 0)
        {
            if(cfg.G1_set.water == 2 && stm.G1_water == 0)
            {
                stm.G1_watering_last = t;
                save = 1;
            }
            else if(cfg.G1_set.water == 2 && stm.G1_water == 1)
            {
                stm_G1_watering(0);
                stm_mc_setn("G1_WATER", 0);
            }
            else if(cfg.G1_set.water != stm.G1_water)
            {
                // Manual On/Off
                stm_G1_watering(cfg.G1_set.water);
                stm_mc_setn("G1_WATER", cfg.G1_set.water);
            }

            if(cfg.G2_set.water == 2 && stm.G2_water == 0)
            {
                stm.G2_watering_last = t;
                save = 1;
            }
            else if(cfg.G2_set.water == 2 && stm.G2_water == 1)
            {
                stm_G2_watering(0);
                stm_mc_setn("G2_WATER", 0);
            }
            else if(cfg.G2_set.water != stm.G2_water)
            {
                // Manual On/Off
                stm_G2_watering(cfg.G2_set.water);
                stm_mc_setn("G2_WATER", cfg.G2_set.water);
            }

            if(cfg.G1_set.vent != 2) stm.G1_vent = cfg.G1_set.vent;
            if(cfg.G2_set.vent != 2) stm.G2_vent = cfg.G2_set.vent;
            if(cfg.G1_set.heat != 2) stm.G1_heat = cfg.G1_set.heat;
            if(cfg.G2_set.heat != 2) stm.G2_heat = cfg.G2_set.heat;
            if(cfg.G1_set.circ != 2) stm.G1_circ = cfg.G1_set.circ;
            if(cfg.G2_set.circ != 2) stm.G2_circ = cfg.G2_set.circ;
            stm.G1_water = cfg.G1_set.water;
            stm.G2_water = cfg.G2_set.water;
            if(save)
            {
                stm_save_time();
            }
            
            stm_mc_setn("G1_VENT_C", cfg.G1_set.vent);
            stm_mc_setn("G1_HEAT_C", cfg.G1_set.heat);
            stm_mc_setn("G1_CIRC_C", cfg.G1_set.circ);
            stm_mc_setn("G1_WATER_C", cfg.G1_set.water);

            stm_mc_setn("G1_PERIOD", cfg.G1_set.period);
            stm_mc_setn("G1_DURATION", cfg.G1_set.duration);

            stm_mc_setn("G2_VENT_C", cfg.G1_set.vent);
            stm_mc_setn("G2_HEAT_C", cfg.G1_set.heat);
            stm_mc_setn("G2_CIRC_C", cfg.G1_set.circ);
            stm_mc_setn("G2_WATER_C", cfg.G1_set.water);

            stm_mc_setn("G2_PERIOD", cfg.G2_set.period);
            stm_mc_setn("G2_DURATION", cfg.G2_set.duration);

        }
        switch(m)
        {
            case ST_IDLE:
            {
                break;
            }
            case ST_DC_12V:
            {
                stm_get_dc(stm.dev, STM_DC_12V, &val);
                stm.adc_12v = val;
                break;
            }
            case ST_DC_BATT:
            {
                stm_get_dc(stm.dev, STM_DC_BATT, &val);
                stm.adc_batt = val;
                break;
            }
            case ST_DC_5V0:
            {
                stm_get_dc(stm.dev, STM_DC_5V0, &val);
                stm.adc_z5v0 = val;
                break;
            }
            case ST_DC_3V3:
            {
                stm_get_dc(stm.dev, STM_DC_3V3, &val);
                stm.adc_z3v3 = val;
                break;
            }
            case ST_DS_ENUM1:
            {
                stm.ds_n = 0;
                set_pca9554(PCA9554_DIS_1W1, 0);
                stm_ds2482_enum(1);
                break;
            }
            case ST_DS_READ1:
            {
                stm_ds2482_read(1);
                set_pca9554(PCA9554_DIS_1W1, 1);
                stm_ds2482_pub(1);
                break;
            }
            case ST_DS_ENUM2:
            {
                set_pca9554(PCA9554_DIS_1W2, 0);
                stm_ds2482_enum(2);
                break;
            }
            case ST_DS_READ2:
            {
                stm_ds2482_read(2);
                set_pca9554(PCA9554_DIS_1W2, 1);
                stm_ds2482_pub(2);
                break;
            }
            case ST_DS_STAT:
            {
                stm_ds_stat();
                break;
            }
            case ST_WATERING:
            {
                stm_watering();
                break;
            }
            default:
            {
                t = 0;
                break;
            }
        }
        if(++m > 10) m = 0;
    }
}

void setup_stm_poll(int id)
{
    int n = poll_fds.n++;
    stm.n_fd = n;
    
    poll_fds.fds[n].fd = getInterruptFS(PIN_PA7);
    poll_fds.fds[n].events = POLLPRI;
    poll_fds.fds[n].revents = 0;
}

void init_stm()
{
    memcached_return rc;
    int v, res;
    int dev;

    memset(&stm, 0, sizeof(stm));
    stm.dev = -1;
    stm.n_fd = -1;
    stm.G1_water = 2;
    stm.G2_water = 2;
    
    digitalWrite(PIN_RST, 0);
    pinMode(PIN_RST, OUTPUT);
    digitalWrite(PIN_RST, 0);

    memcached_create(&stm.mc);
    rc = memcached_server_add_unix_socket(&stm.mc, cfg.memcached);
    if(rc != MEMCACHED_SUCCESS)
    {
        DBG("Error: Create memcached object failed");
        return;
    }
    
    dev = stm_open(0, STM_SLAVE_ADDR);
    if(dev < 0)
    {
        DBG("Error: stm_open() failed %d!", errno);
        return;
    }
    stm.dev = dev;

    stm_G1_watering(0);
    stm_G2_watering(0);

    pinMode (PIN_PA7, INPUT);
    // Read interrupt lane and check it has correct logic level
    v = digitalRead(PIN_PA7);
    if(v != 1)
    {
        DBG("Error: PA7 lane has incorrect logic level: 0");
        return;
    }

    // TODO: Enable interrupt
    // wiringPiISR(PIN_PA7, INT_EDGE_FALLING, &gpioInterrupt1);
    
    // Turn OFF 1W ports
    set_pca9554(PCA9554_DIS_1W1, 1);
    set_pca9554(PCA9554_DIS_1W2, 1);

    stm_get_time();

}

void close_stm()
{
    if(stm.dev > 0) close(stm.dev);   
    stm.dev = -1;
}
