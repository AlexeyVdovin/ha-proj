#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include "config.h"
#include "minini/minIni.h"
#include "wiringPi/wiringPi.h"
#include "uplink.h"
#include "mcp23017.h"
#include "pca9554.h"
#include "stm.h"


#define array_sz(a)  (sizeof(a)/sizeof((a)[0]))
const char* cfg_name = "green.conf";
config_t cfg;
volatile int do_exit = 0;

static void read_config(const char* name)
{
    int i, l, n;
    char str[100] = "";
    char section[100] = "";

    memset(&cfg, 0, sizeof(cfg));

}

static inline int events_poll()
{
    return poll(poll_fds.fds, poll_fds.n, POLL_TIMEOUT);
}


int main(int argc, char* argv[])
{
    int i;
    char str[32];
    
    if(argc > 1)
    {
        cfg_name = argv[1];
    }
    read_config(cfg_name);
    
    wiringPiSetup();

    init_uplink();
    init_stm(0);
    init_pca9554(0);

    setup_stm_poll();
    setup_uplink_poll();
    
    while(!do_exit)
    {
        if(events_poll() > 0)
        {
            //handle_mcp23017();
        }
        handle_stm();
    }

    return 0;
}
