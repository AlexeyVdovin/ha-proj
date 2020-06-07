#ifndef _UPLINK_H_
#define _UPLINK_H_

#include <poll.h>

#include "mqtt.h"

typedef enum
{
    SOC_NONE = 0,
    SOC_CONNECTING,
    SOC_CONNECTED,
    SOC_DISCONNECTED
} t_soc_st;

typedef struct
{
    struct pollfd fds[MAX_POLL_FDS];
    int n;
} poll_t;

extern poll_t poll_fds;

void init_uplink();
void setup_uplink_poll();

void handle_mqtt();
void send_mqtt(char* event, char* value);
    
#endif
