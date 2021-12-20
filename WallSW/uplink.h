#ifndef _UPLINK_H_
#define _UPLINK_H_

#include <poll.h>

#include "mqtt.h"

#define MAX_FILTERS     32
#define MAX_HA_ENTITIES 128

typedef enum
{
    SOC_NONE = 0,
    SOC_CONNECTING,
    SOC_CONNECTED,
    SOC_DISCONNECTED,
    SOC_TIMEOUT
} t_soc_st;

typedef struct
{
    const char* unique_id;
    const char* type;
    const char* name;
    const char* cls;
    const char* units;
} ha_entity_t;

typedef struct
{
    const char* name;
    const char* model;
    const char* ver;
} ha_device_t;

typedef struct
{
    int n;
    ha_device_t* dev[MAX_HA_ENTITIES];
    ha_entity_t* ent[MAX_HA_ENTITIES];
} ha_registry_t;

typedef void (*msgfn_t)(int param, const char* message, size_t message_len);

typedef struct
{
    struct pollfd fds[MAX_POLL_FDS];
    int n;
} poll_t;

extern poll_t poll_fds;

void init_uplink();
void setup_uplink_poll();

void handle_mqtt();
void send_mqtt(const char* event, char* value);
void mqtt_send_status(ha_entity_t* entity, char* status);

void set_uplink_filter(char* filter, msgfn_t cb, int param);
void ha_register(ha_device_t* device, ha_entity_t* entity);

#endif
