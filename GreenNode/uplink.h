#ifndef _UPLINK_H_
#define _UPLINK_H_

#include <poll.h>


typedef void (*msgfn_t)(int param, const char* message, size_t message_len);

typedef struct
{
    struct pollfd fds[MAX_POLL_FDS];
    int n;
} poll_t;

extern poll_t poll_fds;

void init_uplink();
void setup_uplink_poll();

#endif