#ifndef _UPLINK_H_
#define _UPLINK_H_

#include <poll.h>

#include "mqtt.h"

#define MAX_FILTERS     32

typedef enum
{
    SOC_NONE = 0,
    SOC_CONNECTING,
    SOC_CONNECTED,
    SOC_DISCONNECTED,
    SOC_TIMEOUT
} t_soc_st;

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

void set_uplink_filter(char* filter, msgfn_t cb, int param);
    
#endif
