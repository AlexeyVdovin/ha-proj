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
#include "ds2482.h"
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

    n = ini_gets("general", "memcached", "", cfg.memcached, sizeof(cfg.memcached)-1, name);
    n = ini_gets("general", "settings", "", cfg.settings, sizeof(cfg.settings)-1, name);

    cfg.n_avg = ini_getl("general", "n_avg", 3, name);
    cfg.circ_delta = ini_getl("general", "circ_delta", 3, name);
    cfg.heating = ini_getl("general", "heating", 14, name);
    cfg.ventilation = ini_getl("general", "ventilation", 28, name);

    n = ini_gets("general", "G1_air", "", cfg.G1_air, sizeof(cfg.G1_air)-1, name); sscanf(cfg.G1_air+16, "%f %f", &cfg.G1_air_c, &cfg.G1_air_k);
    n = ini_gets("general", "G1_ground", "", cfg.G1_ground, sizeof(cfg.G1_ground)-1, name); sscanf(cfg.G1_ground+16, "%f %f", &cfg.G1_ground_c, &cfg.G1_ground_k);
    n = ini_gets("general", "G2_air", "", cfg.G2_air, sizeof(cfg.G2_air)-1, name); sscanf(cfg.G2_air+16, "%f %f", &cfg.G2_air_c, &cfg.G2_air_k);
    n = ini_gets("general", "G2_ground", "", cfg.G2_ground, sizeof(cfg.G2_ground)-1, name); sscanf(cfg.G2_ground+16, "%f %f", &cfg.G2_ground_c, &cfg.G2_ground_k);
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
    init_pca9554();
    init_ds2482();
    init_stm();

    setup_uplink_poll();
    setup_stm_poll();

    while(!do_exit)
    {
        if(events_poll() > 0)
        {
//            handle_ds2482();
        }
        handle_stm();
    }

    return 0;
}
