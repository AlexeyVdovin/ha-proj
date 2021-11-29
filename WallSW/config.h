#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LIGHTS_MAX_DEV   2
#define POWER_MAX_DEV  4


#define MAX_POLL_FDS    32
#define POLL_TIMEOUT    1000


#define PIN_INT     11
#define PIN_RST     7
#define PIN_PA7     1


#define DBG(format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

typedef struct
{
    int   id;
} wallsw_t;

typedef struct
{
    int   id;
} lights_t;

typedef struct
{
    int id;
} power_t;

typedef struct
{
    int  id;
    char sensor[20][20];
    int  pid1_p_gain;
    int  pid1_i_gain;
    int  pwm1_min;
    int  pwm1_max;
    int  hwater_min;
    int  hwater_max;
} boiler_t;

typedef struct
{
    char  client_name[32];
    char  mqtt_topic[64];
    char  uplink_ip[32];
    int   uplink_port;
    wallsw_t  sw;
    int       n_lts;
    lights_t  lts[LIGHTS_MAX_DEV];
    int       n_pwr;
    power_t   pwr[POWER_MAX_DEV];
    int       n_bol;
    boiler_t  boiler;
} config_t;

extern config_t cfg;

#endif