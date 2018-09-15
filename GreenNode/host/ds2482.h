#ifndef __DS2482_H__
#define __DS2482_H__

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
int ds2482_reset(int dev);
int ds2482_set_conf(int dev, uint8_t conf);
int ds2482_get_status(int dev, uint8_t* value);
int ds2482_get_data(int dev, uint8_t* value);
int ds2482_get_conf(int dev, uint8_t* conf);
int ds2482_read(int dev, uint8_t* value);
int ds2482_pool(int dev, uint8_t mask, uint8_t* value, int count);

/* I2C gateway 1W operations */
int ds2482_1w_reset(int dev);
int ds2482_1w_1b(int dev, char bit);
int ds2482_1w_wbyte(int dev, uint8_t value);
int ds2482_1w_rbyte(int dev);
int ds2482_1w_triplet(int dev, char bit);

/* I2C gateway 1W high level operations */
int ds2482_1w_search(int dev, ow_serch_t* search);
int ds2482_1w_skip(int dev);
int ds2482_1w_match(int dev, uint8_t addr[8]);

/* I2C gateway DS18B20 operations */
int ds2482_ds18b20_convert(int dev);
int ds2482_ds18b20_read_power(int dev, uint8_t* value);
int ds2482_ds18b20_read_scratchpad(int dev, uint8_t* data);
int ds2482_ds18b20_read_eeprom(int dev);
int ds2482_ds18b20_write_configuration(int dev, uint8_t* conf);
int ds2482_ds18b20_write_eeprom(int dev);


#endif /* __DS2482_H__ */