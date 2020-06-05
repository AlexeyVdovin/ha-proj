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

#define STM_CONTROL 77 // TBD


typedef struct
{
    int dev;
    int n_fd;
    uint8_t control;
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

int stm_set_control(int dev, uint8_t val)
{
    union i2c_smbus_data data;
    data.byte = val;
    return i2c_io(dev, I2C_SMBUS_WRITE, STM_CONTROL, I2C_SMBUS_BYTE_DATA, &data);
}

// stm32 Interrupt PA07 pin
static void gpioInterrupt1(void) 
{ 
    DBG("Interrupt: stm32");

}

void handle_stm()
{
    uint8_t c;
    int revents = poll_fds.fds[stm.n_fd].revents;
    
    if(revents)
    {
        read(getInterruptFS(PIN_PA7), &c, 1);
        poll_fds.fds[stm.n_fd].revents = 0;
        gpioInterrupt1();
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
    
    digitalWrite(PIN_RST, 0);
    pinMode (PIN_RST, OUTPUT);
    digitalWrite(PIN_RST, 0);
    
    dev = stm_open(0, 0x40 + cfg.dev_id);

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
    
    //res = stm_enable_dcdc(dev, 1);
}

void close_stm()
{
    if(stm.dev > 0) close(stm.dev);   
    stm.dev = -1;
}
