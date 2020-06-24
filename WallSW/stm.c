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
#include "stm.h"
#include "wiringPi/wiringPi.h"

enum
{
    ST_IDLE = 0,
    ST_DC_12V,
    ST_DC_BATT,
    ST_DC_5V0,
    ST_DC_3V3,
    ST_DC_VCC,
    ST_DC_REF,
    ST_TEMPERATURE
};

static const char STR_DC_12V[]      = "dc:12V";
static const char STR_DC_BAT[]      = "dc:BATT";
static const char STR_DC_5V0[]      = "dc:5V0";
static const char STR_DC_3V3[]      = "dc:3V3";
static const char STR_DC_VCC[]      = "dc:VCC";
static const char STR_DC_REF[]      = "dc:REF";
static const char STR_TEMPERATURE[] = "TEMPERATURE";


typedef struct
{
    int      n_ints;
    int      dev;
    int      n_fd;
    int      state;
    uint16_t control;
} stm_t;

stm_t stm;

int stm_open(uint8_t bus, uint8_t adr)
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

int stm_set_control(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_CONTROL, I2C_SMBUS_WORD_DATA, &data);
}

int stm_get_status(int dev, uint32_t *val)
{
    int res;
    union i2c_smbus_data data;
    
    do
    {
        res = i2c_io(dev, I2C_SMBUS_READ, STM_STATUS, I2C_SMBUS_WORD_DATA, &data);
        if(res < 0) break;
        *val = (0x0FFFF & data.word) << 16;
        res = i2c_io(dev, I2C_SMBUS_READ, STM_STATUS+2, I2C_SMBUS_WORD_DATA, &data);
        if(res < 0) break;
        *val |= 0x0FFFF & data.word;
    } while(0);
    return res;
}

int stm_get_dc(int dev, uint8_t in, uint16_t *val)
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

void handle_stm()
{
    static int s = -20;
    int t;
    uint8_t c;
    uint16_t val;
    int revents = poll_fds.fds[stm.n_fd].revents;
    const char* event;
    char str[16];
    
    if(revents)
    {
        poll_fds.fds[stm.n_fd].revents = 0;
        gpioInterrupt1();
        read(getInterruptFS(PIN_PA7), &c, 1);
    }

#if 0    
    if(s < 0)
    {
        s++;
        return;
    }
#endif
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
                stm_get_dc(stm.dev, STM_DC_12V, &val);
                break;
            }
            case ST_DC_BATT:
            {
                event = STR_DC_BAT;
                stm_get_dc(stm.dev, STM_DC_BATT, &val);
                break;
            }
            case ST_DC_5V0:
            {
                event = STR_DC_5V0;
                stm_get_dc(stm.dev, STM_DC_5V0, &val);
                break;
            }
            case ST_DC_3V3:
            {
                event = STR_DC_3V3;
                stm_get_dc(stm.dev, STM_DC_3V3, &val);
                break;
            }
            case ST_DC_VCC:
            {
                event = STR_DC_VCC;
                stm_get_dc(stm.dev, STM_DC_VCC, &val);
                break;
            }
            case ST_DC_REF:
            {
                event = STR_DC_REF;
                stm_get_dc(stm.dev, STM_DC_REF, &val);
                break;
            }
            case ST_TEMPERATURE:
            {
                event = STR_TEMPERATURE;
                stm_get_dc(stm.dev, STM_TEMPR, &val);
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
            sprintf(str, "%d", val);
            send_mqtt(event, str);
        }
    }
}

void setup_stm_poll()
{
    int n = poll_fds.n++;
    stm.n_fd = n;
    
    poll_fds.fds[n].fd = getInterruptFS(PIN_PA7);
    poll_fds.fds[n].events = POLLPRI;
    poll_fds.fds[n].revents = 0;
}

void init_stm()
{
    int v, res;
    int dev;
    
    stm.dev = -1;
    stm.n_fd = -1;
    stm.control = 0;
    stm.state = 0;
    
    digitalWrite(PIN_RST, 0);
    pinMode (PIN_RST, OUTPUT);
    digitalWrite(PIN_RST, 0);
    
    dev = stm_open(0, 0x10 + cfg.sw.id);

    if(dev < 0)
    {
        DBG("Error: stm_open() failed %d!", errno);
        return;
    }
    stm.dev = dev;

    pinMode (PIN_PA7, INPUT);
    // Read interrupt lane and check it has correct logic level
    v = digitalRead(PIN_PA7);
    if(v != 1)
    {
        DBG("Error: PA7 lane has incorrect logic level: 0");
        return;
    }
    wiringPiISR(PIN_PA7, INT_EDGE_FALLING, &gpioInterrupt1);
    
    stm.control = STM_DCDC_EN;
    res = stm_set_control(dev, stm.control);
}

void close_stm()
{
    if(stm.dev > 0) close(stm.dev);   
    stm.dev = -1;
}
