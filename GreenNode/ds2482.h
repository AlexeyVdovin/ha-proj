#ifndef __DS2482_H__
#define __DS2482_H__

#include <inttypes.h>

#define DS2482_SLAVE_ADDR  0x18

#define DS2482_CMD_TRIPLET          0x78
#define DS2482_CMD_SINGLEBIT        0x87
#define DS2482_CMD_READBYTE         0x96
#define DS2482_CMD_WRITEBYTE        0xA5
#define DS2482_CMD_RESETWIRE        0xB4
#define DS2482_CMD_WRITECONFIG      0xD2
#define DS2482_CMD_RESET            0xF0

#define OW_CMD_SKIP                 0xCC
#define OW_CMD_MATCH                0x55
#define OW_CMD_SEARCH               0xF0

#define DS1820_CMD_CONVERT          0x44
#define DS1820_CMD_WR_SCRATCHPAD    0x4E
#define DS1820_CMD_RD_SCRATCHPAD    0xBE
#define DS1820_CMD_PPD              0xB4
#define DS1820_CMD_EE_WRITE         0x48
#define DS1820_CMD_EE_RECALL        0xB8


typedef struct
{
    uint8_t addr[8];
    uint8_t next;
} ow_serch_t;


int ds2482_open(uint8_t bus, uint8_t adr);

/* I2C gateway LL operations */
int ds2482_reset();
int ds2482_set_conf(uint8_t conf);
int ds2482_get_status(uint8_t* value);
int ds2482_get_data(uint8_t* value);
int ds2482_get_conf(uint8_t* conf);
int ds2482_read(uint8_t* value);
int ds2482_pool(uint8_t mask, uint8_t* value, int count);

/* I2C gateway 1W operations */
int ds2482_1w_reset();
int ds2482_1w_1b(char bit);
int ds2482_1w_wbyte(uint8_t value);
int ds2482_1w_rbyte();
int ds2482_1w_triplet(char bit);

/* I2C gateway 1W high level operations */
int ds2482_1w_search(ow_serch_t* search);
int ds2482_1w_skip();
int ds2482_1w_match(uint8_t addr[8]);

/* I2C gateway DS18B20 operations */
int ds2482_ds18b20_convert();
int ds2482_ds18b20_read_power(uint8_t* value);
int ds2482_ds18b20_read_scratchpad(uint8_t* data);
int ds2482_ds18b20_read_eeprom();
int ds2482_ds18b20_write_configuration(uint8_t* conf);
int ds2482_ds18b20_write_eeprom();

int ds2482_ds18b20_scan_temp();

void init_ds2482();
void close_ds2482();

#endif /* __DS2482_H__ */