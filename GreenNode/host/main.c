#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command,
                                     int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(file, I2C_SMBUS, &args);
}

static inline __s32 i2c_smbus_read_word_data(int file, __u8 command)
{
    union i2c_smbus_data data;
    if(i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data))
        return -1;
    else
        return 0x0FFFF & data.word;
}

int set_slave_addr(int file, int address)
{
    if (ioctl(file, I2C_SLAVE, address) < 0)
    {
        printf("Error: Could not set address to 0x%02x: %s\n", address, strerror(errno));
        return -errno;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    char* devname = "/dev/i2c-0";
    int dev = open(devname, O_RDWR);
    int res = 0;

    if(dev < 0)
    {
        printf("Error %d: Can't open i2c device %s\n", errno, devname);
        exit(1);
    }

    res = set_slave_addr(dev, 0x16);

    if(res < 0)
    {
        printf("Error %d: Can't assign slave i2c address.\n", errno);
        exit(1);
    }


    res = i2c_smbus_read_word_data(dev, 0x00);

    if(res < 0)
    {
        printf("Error %d: Can't read from i2c device.\n", errno);
        exit(1);
    }

    printf("0x%04x\n", res);

    return 0;
}


