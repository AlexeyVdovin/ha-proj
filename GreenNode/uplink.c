#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "uplink.h"


poll_t      poll_fds;

static int poll_fdn = -1;

void init_uplink()
{
    memset(&poll_fds, sizeof(poll_fds), 0);
}

void setup_uplink_poll()
{
    int n = poll_fds.n++;
    poll_fds.fds[n].fd = -1;
    poll_fds.fds[n].events = 0;
    poll_fds.fds[n].revents = 0;
    poll_fdn = n;
}

