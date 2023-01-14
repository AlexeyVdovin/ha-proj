#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
    Gx_VENT     - Ventilation status
    Gx_HEAT     - Heating status
    Gx_CIRC     - Circulation status
    Gx_WATER    - Watering status
    Gx_WATER_T  - Last watering time

    Gx_VENT_C   - Ventilation control
    Gx_HEAT_C   - Heating control
    Gx_CIRC_C   - Circulation control
    Gx_WATER_C  - Watering control

    Gx_PERIOD   - Watering period
    Gx_DURATION - Watering duration


*/


#define MAX_POLL_FDS    32
#define POLL_TIMEOUT    1000


#define PIN_INT     11
#define PIN_RST     7
#define PIN_PA7     1


#define DBG(format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

typedef struct
{
    char  vent;     // 0 - OFF, 1 - ON, 2 - AUTO
    char  heat;     // 0 - OFF, 1 - ON, 2 - AUTO
    char  circ;     // 0 - OFF, 1 - ON, 2 - AUTO
    char  water;    // 0 - OFF, 1 - ON, 2 - AUTO
    char  period;   // in minutes
    char  duration; // in minutes
} settings_t;

typedef struct
{
    char  client_name[32];
    char  memcached[64];
    char  settings[64];
    char  watering[64];
    char  G1_air[32];
    float G1_air_k;
    float G1_air_c;
    char  G1_ground[32];
    float G1_ground_k;
    float G1_ground_c;
    char  G2_air[32];
    float G2_air_k;
    float G2_air_c;
    char  G2_ground[32];
    float G2_ground_k;
    float G2_ground_c;
    int   n_avg;
    int   heating;
    int   ventilation;
    int   circ_delta;
    settings_t G1_set;
    settings_t G2_set;
} config_t;

extern config_t cfg;

#endif