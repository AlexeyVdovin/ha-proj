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
#include "boiler.h"

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

    n = ini_gets("general", "boiler", "0", str, array_sz(str), name);
    DBG("boiler: '%s'", str);
    cfg.n_bol = atol(str);

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
        n = ini_gets(section, "id", "0", str, array_sz(str), name);
        DBG("%s.id: '%s'", section, str);
        cfg.lts[i].id = atol(str);
    }        
    for(i = 0; i < cfg.n_pwr; ++i)
    {
        snprintf(section, sizeof(section), "power-%d", i);
        n = ini_gets(section, "id", "0", str, array_sz(str), name);
        DBG("%s.id: '%s'", section, str);
        cfg.lts[i].id = atol(str);
    }        
    if(cfg.n_bol > 0)
    {
        n = ini_gets("boiler", "id", "0", str, array_sz(str), name);
        DBG("boiler.id: '%s'", str);
        cfg.boiler.id = atol(str);
        // TODO: Add sensor SERIAL IDs for every sensor type
        n = ini_gets("boiler", "boiler_temp_1", "", cfg.boiler.sensor[OW_BOILER_TEMP1], 20, name);
        n = ini_gets("boiler", "boiler_temp_2", "", cfg.boiler.sensor[OW_BOILER_TEMP2], 20, name);
        n = ini_gets("boiler", "boiler_in", "", cfg.boiler.sensor[OW_BOILER_IN], 20, name);
        n = ini_gets("boiler", "boiler_out", "", cfg.boiler.sensor[OW_BOILER_OUT], 20, name);
        n = ini_gets("boiler", "boiler_ret", "", cfg.boiler.sensor[OW_BOILER_RET], 20, name);
        n = ini_gets("boiler", "ht_floor_in", "", cfg.boiler.sensor[OW_FLOOR_IN], 20, name);
        n = ini_gets("boiler", "ht_floor_out", "", cfg.boiler.sensor[OW_FLOOR_OUT], 20, name);
        n = ini_gets("boiler", "ht_floor_ret", "", cfg.boiler.sensor[OW_FLOOR_RET], 20, name);
        n = ini_gets("boiler", "ch_pipe_out", "", cfg.boiler.sensor[OW_PIPE_OUT], 20, name);
        n = ini_gets("boiler", "ch_pipe_ret", "", cfg.boiler.sensor[OW_PIPE_RET], 20, name);
        n = ini_gets("boiler", "gas_heat_in", "", cfg.boiler.sensor[OW_HEAT_IN], 20, name);
        n = ini_gets("boiler", "gas_heat_out", "", cfg.boiler.sensor[OW_HEAT_OUT], 20, name);
        n = ini_gets("boiler", "gas_hwater_in", "", cfg.boiler.sensor[OW_HWATER_IN], 20, name);
        n = ini_gets("boiler", "gas_hwater_out", "", cfg.boiler.sensor[OW_HWATER_OUT], 20, name);
        n = ini_gets("boiler", "ambient", "", cfg.boiler.sensor[OW_AMBIENT], 20, name);

        n = ini_gets("boiler", "pid1_p_gain", "0", str, array_sz(str), name);
        DBG("boiler.pid1_p_gain: '%s'", str);
        cfg.boiler.pid1_p_gain = atol(str);

        n = ini_gets("boiler", "pid1_i_gain", "0", str, array_sz(str), name);
        DBG("boiler.pid1_i_gain: '%s'", str);
        cfg.boiler.pid1_i_gain = atol(str);

        n = ini_gets("boiler", "pwm1_min", "0", str, array_sz(str), name);
        DBG("boiler.pwm1_min: '%s'", str);
        cfg.boiler.pwm1_min = atol(str);

        n = ini_gets("boiler", "pwm1_max", "0", str, array_sz(str), name);
        DBG("boiler.pwm1_max: '%s'", str);
        cfg.boiler.pwm1_max = atol(str);

        n = ini_gets("boiler", "hwater_min", "45", str, array_sz(str), name);
        DBG("boiler.hwater_min: '%s'", str);
        cfg.boiler.hwater_min = atol(str);

        n = ini_gets("boiler", "hwater_max", "50", str, array_sz(str), name);
        DBG("boiler.hwater_max: '%s'", str);
        cfg.boiler.hwater_max = atol(str);

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
    init_boiler();
    set_uplink_filter("boiler", msg_boiler, 0);


    for(i = 0; i < cfg.n_lts; ++i)
    {
        init_pca9454(cfg.lts[i].id);
        snprintf(str, sizeof(str)-1, "lts-%d", i);
        set_uplink_filter(str, msg_pca9454, cfg.lts[i].id);
    }        
    
    setup_mcp23017_poll();
    setup_stm_poll();
    setup_boiler_poll();
    setup_uplink_poll();
    
    while(!do_exit)
    {
        if(events_poll() > 0)
        {
            handle_mcp23017();
            handle_boiler();
        }
        handle_stm();
        handle_mqtt();
    }

    return 0;
}
