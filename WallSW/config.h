#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_POLL_FDS    20
#define POLL_TIMEOUT    1000


#define PIN_INT     11
#define PIN_RST     7
#define PIN_PA7     1


#define DBG(format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

typedef struct
{
    char  client_name[32];
    char  mqtt_topic[64];
    char  uplink_ip[32];
    int   uplink_port;
    int   dev_id;
    
} config_t;

extern config_t cfg;

#endif