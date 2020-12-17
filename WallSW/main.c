#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "config.h"
#include "minini/minIni.h"
#include "wiringPi/wiringPi.h"
#include "uplink.h"
#include "mcp23017.h"
#include "pca9454.h"
#include "stm.h"

#define array_sz(a)  (sizeof(a)/sizeof((a)[0]))
const char* cfg_name = "wallsw.conf";
config_t cfg;
volatile int do_exit = 0;

static void read_config(const char* name)
{
    int i, n;
    char str[100] = "";
    char section[100] = "";
    
    n = ini_gets("general", "lights", "0", str, array_sz(str), name);
    DBG("lights: '%s'", str);
    cfg.n_lts = atol(str);
    
    n = ini_gets("general", "powersw", "0", str, array_sz(str), name);
    DBG("powersw: '%s'", str);
    cfg.n_pwr = atol(str);

    n = ini_gets("mqtt", "client", "sgw2.2", str, array_sz(str), name);
    DBG("client_name: '%s'", str);
    strncpy(cfg.client_name, str, sizeof(cfg.client_name));

    n = ini_gets("mqtt", "topic", "/nn/kan25/floor-02", str, array_sz(str), name);
    DBG("mqtt_topic: '%s'", str);
    strncpy(cfg.mqtt_topic, str, sizeof(cfg.mqtt_topic));

    n = ini_gets("mqtt", "ip", "127.0.0.1", str, array_sz(str), name);
    DBG("ip: '%s'", str);
    strncpy(cfg.uplink_ip, str, sizeof(cfg.uplink_ip));

    n = ini_gets("mqtt", "port", "1883", str, array_sz(str), name);
    DBG("port: '%s'", str);
    cfg.uplink_port = atol(str);

    for(i = 0; i < cfg.n_lts; ++i)
    {
        snprintf(section, sizeof(section), "lights-%d", i);
        n = ini_gets(section, "id", "1", str, array_sz(str), name);
        DBG("%s.id: '%s'", section, str);
        cfg.lts[i].id = atol(str);
    }        
    for(i = 0; i < cfg.n_pwr; ++i)
    {
        snprintf(section, sizeof(section), "power-%d", i);
        n = ini_gets(section, "id", "1", str, array_sz(str), name);
        DBG("%s.id: '%s'", section, str);
        cfg.lts[i].id = atol(str);
    }        
    
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
    init_stm();
    init_mcp23017();
    set_uplink_filter("mcp", msg_mcp23017, 0);

    for(i = 0; i < cfg.n_lts; ++i)
    {
        init_pca9454(cfg.lts[i].id);
        snprintf(str, sizeof(str)-1, "lts-%d", i);
        set_uplink_filter(str, msg_pca9454, cfg.lts[i].id);
    }        
    
    setup_mcp23017_poll();
    setup_stm_poll();
    setup_uplink_poll();
    
    while(!do_exit)
    {
        if(events_poll() > 0)
        {
            handle_mcp23017();
        }
        handle_stm();
        handle_mqtt();
    }

    return 0;
}
