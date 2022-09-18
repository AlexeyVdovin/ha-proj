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
#include <sys/timerfd.h>
#include <time.h>

#include "config.h"
#include "uplink.h"
#include "mcp23017.h"
#include "wiringPi/wiringPi.h"

extern ha_device_t device;

static ha_entity_t en_switch[16] =
{
    {
        "floor2_bed_left_switch1",
        "binary_sensor",
        "Bed Left Switch 1 (NA)",
        "power",
        NULL
    },
    {
        "floor2_bed_left_switch2",
        "binary_sensor",
        "Bed Left Switch 2",
        "power",
        NULL
    },
    {
        "floor2_bed_right_switch1",
        "binary_sensor",
        "Bed Right Switch 1",
        "power",
        NULL
    },
    {
        "floor2_bed_right_switch2",
        "binary_sensor",
        "Bed Right Switch 2 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_4",
        "binary_sensor",
        "Reserved Switch 4 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_5",
        "binary_sensor",
        "Reserved Switch 5 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_6",
        "binary_sensor",
        "Reserved Switch 6 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_7",
        "binary_sensor",
        "Reserved Switch 7 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_8",
        "binary_sensor",
        "Reserved Switch 8 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_9",
        "binary_sensor",
        "Reserved Switch 9 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_10",
        "binary_sensor",
        "Reserved Switch 10 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_11",
        "binary_sensor",
        "Reserved Switch 11 (NA)",
        "power",
        NULL
    },
    {
        "floor2_corridor_night_switch",
        "binary_sensor",
        "2: Corridor Night Switch",
        "power",
        NULL
    },
    {
        "floor2_corridor_bright_switch",
        "binary_sensor",
        "2: Corridor Bright Switch",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_14",
        "binary_sensor",
        "Reserved Switch 14 (NA)",
        "power",
        NULL
    },
    {
        "floor2_reserved_switch2_15",
        "binary_sensor",
        "Reserved Switch 15 (NA)",
        "power",
        NULL
    }
};

typedef struct
{
    int      n_ints;
    int      dev;
    int      n_fd;
    int      tmr;
    uint16_t tmr_pins;
    uint16_t pub_pins;
    struct itimerspec tmr_value;
    
} mcp23017_t;

mcp23017_t mcp;

int mcp23017_open(uint8_t bus, uint8_t adr)
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

int mcp23017_set_conf(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IOCONA, I2C_SMBUS_BYTE_DATA, &data);
}

int mcp23017_set_dir(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_IODIRA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_set_defval(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_DEFVALA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_set_cmpval(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_INTCONA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_set_inten(int dev, uint16_t val)
{
    union i2c_smbus_data data;
    data.word = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, MCP23017_GPINTENA, I2C_SMBUS_WORD_DATA, &data);
}

int mcp23017_get_conf(int dev, uint8_t* conf)
{
    int res = -1;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, MCP23017_IOCONA, I2C_SMBUS_BYTE_DATA, &data);
    if(res >= 0) *conf = 0x0FF & data.byte;
    return res;
}

int mcp23017_get_pins(int dev, uint16_t* val)
{
    int res = -1;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, MCP23017_GPIOA, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;
    return res;
}

int mcp23017_get_latch(int dev, uint16_t* val)
{
    int res = -1;
    union i2c_smbus_data data;

    res = i2c_io(dev, I2C_SMBUS_READ, MCP23017_INTCAPA, I2C_SMBUS_WORD_DATA, &data);
    if(res >= 0) *val = 0x0FFFF & data.word;
    return res;
}

static void notify()
{
    int i;
    uint16_t mask, val = mcp.tmr_pins;
    char event[16];
    
    // DBG("notify: 0x%04X -> 0x%04X", mcp.pub_pins, mcp.tmr_pins);
    
    for(i = 0; i < 16; ++i)
    {
        mask = 1 << i;
        if((mcp.pub_pins&mask) != (val&mask))
        {
            // sprintf(event, "sw:%d", i);
            // send_mqtt(event, (val&mask) ? "1" : "0");
            mqtt_send_status(&en_switch[i], (val&mask) ? "ON" : "OFF");

        }
    }
    mcp.pub_pins = val;
}

// mcp23017 Interrupt pin
static void gpioInterrupt0(void) 
{
    int res;
    uint16_t p;
    int dev = mcp.dev;
    
    //DBG("Interrupt: mcp23017");
    
    mcp.n_ints++;
    
    // Read current inputs
    res = mcp23017_get_pins(dev, &p);
    
    // Contact bounce suppression
    mcp.tmr_value.it_value.tv_nsec = 20 * 1000000; // 20ms
    timerfd_settime(mcp.tmr, 0, &mcp.tmr_value, NULL);
    mcp.tmr_pins = p;
}

// Contact bounce passed
static void tmrInterrupt0(void)
{
    int res;
    uint16_t p;
    int dev = mcp.dev;

    // Read current inputs
    res = mcp23017_get_pins(dev, &p);

    // DBG("Tmr INT: 0x%04X, 0x%04X, 0x%04X, %d", mcp.tmr_pins, p, mcp.pub_pins, mcp.n_ints);

    if(mcp.tmr_pins != p)
    {
        // Contact bounce suppression
        mcp.tmr_value.it_value.tv_nsec = 20 * 1000000; // 20ms
        timerfd_settime(mcp.tmr, 0, &mcp.tmr_value, NULL);
        mcp.tmr_pins = p;
    }
    else
    {
        if(mcp.pub_pins != mcp.tmr_pins) notify();
    }
}

void init_mcp23017()
{
    int v;
    int dev;
    int res;
    uint16_t p;
    
    mcp.dev  = -1;
    mcp.n_fd = -1;
    mcp.tmr = -1;
    mcp.n_ints = 0;
    
    mcp.tmr_value.it_value.tv_sec = 0;
    mcp.tmr_value.it_value.tv_nsec = 900 * 1000000;
    mcp.tmr_value.it_interval.tv_sec = 0;
    mcp.tmr_value.it_interval.tv_nsec = 0;

    dev = mcp23017_open(0, 0x20 + cfg.sw.id);

    if(dev < 0)
    {
        DBG("Error: mcp23017_open() failed %d!", errno);
        return;
    }
    mcp.dev = dev;

    // The INT pins are internally connected
    // INT output pin - Active-low
    res = mcp23017_set_conf(dev, 0x40);

    // All inputs
    res = mcp23017_set_dir(dev, 0xffff);

    // Default value 0x0000
    res = mcp23017_set_defval(dev, 0x0000);

    // Compare inputs with it's previous values
    res = mcp23017_set_cmpval(dev, 0x0000);
    
    // Read current inputs
    res = mcp23017_get_pins(dev, &p);
    mcp.tmr_pins = p;
    mcp.pub_pins = ~p;
    
    pinMode (PIN_INT, INPUT);
    // Read interrupt lane and check it has correct logic level
    v = digitalRead(PIN_INT);
    if(v != 1)
    {
        DBG("Error: Interrupt lane has incorrect logic level: 0");
        return;
    }
    wiringPiISR(PIN_INT, INT_EDGE_FALLING, &gpioInterrupt0);

    ha_register_binary(&device, &en_switch[0]);
    ha_register_binary(&device, &en_switch[1]);
    ha_register_binary(&device, &en_switch[2]);
    ha_register_binary(&device, &en_switch[3]);
    ha_register_binary(&device, &en_switch[4]);
    ha_register_binary(&device, &en_switch[5]);
    ha_register_binary(&device, &en_switch[6]);
    ha_register_binary(&device, &en_switch[7]);
    ha_register_binary(&device, &en_switch[8]);
    ha_register_binary(&device, &en_switch[9]);
    ha_register_binary(&device, &en_switch[10]);
    ha_register_binary(&device, &en_switch[11]);
    ha_register_binary(&device, &en_switch[12]);
    ha_register_binary(&device, &en_switch[13]);
    ha_register_binary(&device, &en_switch[14]);
    ha_register_binary(&device, &en_switch[15]);
    
    mcp.tmr = timerfd_create(CLOCK_MONOTONIC, 0);
    // Set timer to publish initial satus
    timerfd_settime(mcp.tmr, 0, &mcp.tmr_value, NULL);
    
    // All interrupts enabled
    res = mcp23017_set_inten(dev, 0xffff);
}

void close_mcp23017()
{
    if(mcp.dev > 0) close(mcp.dev);
    mcp.dev = -1;
    if(mcp.tmr > 0) close(mcp.tmr);
    mcp.tmr = -1;
}

void handle_mcp23017()
{
    uint8_t c;
    uint64_t exp;
    int revents;
    
    revents = poll_fds.fds[mcp.n_fd].revents;
    if(revents)
    {
        poll_fds.fds[mcp.n_fd].revents = 0;
        gpioInterrupt0();
        read(getInterruptFS(PIN_INT), &c, 1);
    }

    revents = poll_fds.fds[mcp.n_fd+1].revents;
    if(revents)
    {
        poll_fds.fds[mcp.n_fd+1].revents = 0;
        read(mcp.tmr, &exp, sizeof(exp));
        tmrInterrupt0();
    }
}

void setup_mcp23017_poll()
{
    int n = poll_fds.n;
    mcp.n_fd = n;
    poll_fds.n += 2;
    
    poll_fds.fds[n].fd = getInterruptFS(PIN_INT);
    poll_fds.fds[n].events = POLLPRI;
    poll_fds.fds[n].revents = 0;
    ++n;
    poll_fds.fds[n].fd = mcp.tmr;
    poll_fds.fds[n].events = POLLIN;
    poll_fds.fds[n].revents = 0;
}

void msg_mcp23017(int param, const char* message, size_t message_len)
{
    
}
