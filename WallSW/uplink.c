#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <time.h>

#include "config.h"
#include "uplink.h"

enum
{
    HA_REG_SENSOR = 0,
    HA_REG_BINARY,
    HA_REG_SWITCH,
    HA_REG_NUMBER,
    HA_REG_UNKNOWN
};

typedef struct
{
    const char*   str;
    msgfn_t cb;
    int     param;
} filter_t;

typedef struct
{
    struct mqtt_client client;
    uint8_t sendbuf[32768];
    uint8_t recvbuf[1024];
    struct sockaddr_in addr;
    socklen_t addr_len;
    t_soc_st  status;
    int       sd;
    int       n_fd;
    int       n_ft;
    filter_t  ft[MAX_FILTERS];
} mqtt_t;


poll_t      poll_fds;
mqtt_t      client;

ha_registry_t registry = { 0 };

static inline struct mqtt_client* mq(mqtt_t* c) { return &c->client; }


static void process_mqtt(const char* topic, size_t topic_len, const char* message, size_t message_len)
{
    int i;
    char match[256];
    
    for(i = 0; i < client.n_ft; ++i)
    {
        snprintf(match, sizeof(match)-1, "%s/%s/cmd", cfg.mqtt_topic, client.ft[i].str);
        if(memcmp(topic, match, topic_len) == 0)
        {
            client.ft[i].cb(client.ft[i].param, message, message_len);
            break;
        }
    }
}

static void mqtt_callback(void** unused, struct mqtt_response_publish *published)
{
    DBG("Received publish('%.*s'): %.*s", (int)published->topic_name_size, (const char*)published->topic_name, (int)published->application_message_size, (const char*)published->application_message);
    process_mqtt((const char*)published->topic_name, published->topic_name_size, (const char*)published->application_message, published->application_message_size);
}

void send_registry()
{
    static int i = 0;
    int error = MQTT_OK;
    char topic[128];
    char config[2048];

    for(; i < registry.n; i++)
    {
        ha_device_t* device = registry.dev[i];
        ha_entity_t* entity = registry.ent[i];

        snprintf(topic, sizeof(topic)-1, "homeassistant/%s/%s/config", entity->type, entity->unique_id);

        switch(registry.type[i])
        {
            case HA_REG_SENSOR:
                snprintf(config, sizeof(config)-1, "{"
                    " \"name\": \"%s\","
                    " \"unique_id\": \"%s\","
                    " \"state_topic\": \"%s/%s/state\","
                    " \"device_class\": \"%s\","
                    " \"unit_of_measurement\": \"%s\","
                    " \"platform\": \"mqtt\","
                    " \"device\": {"
                        " \"identifiers\": [\"%s\"],"
                        " \"name\": \"%s\","
                        " \"manufacturer\": \"AtHome\","
                        " \"model\": \"%s\","
                        " \"sw_version\": \"%s\""
                        " }"
                    " }",
                    entity->name, 
                    entity->unique_id, 
                    cfg.mqtt_topic, entity->unique_id,
                    entity->cls,
                    entity->units,
                    device->name,
                    device->name,
                    device->model,
                    device->ver);
                break;
            case HA_REG_BINARY:
                snprintf(config, sizeof(config)-1, "{"
                    " \"name\": \"%s\","
                    " \"unique_id\": \"%s\","
                    " \"state_topic\": \"%s/%s/state\","
                    " \"device_class\": \"%s\","
                    " \"platform\": \"mqtt\","
                    " \"device\": {"
                        " \"identifiers\": [\"%s\"],"
                        " \"name\": \"%s\","
                        " \"manufacturer\": \"AtHome\","
                        " \"model\": \"%s\","
                        " \"sw_version\": \"%s\""
                        " }"
                    " }",
                    entity->name, 
                    entity->unique_id, 
                    cfg.mqtt_topic, entity->unique_id,
                    entity->cls,
                    device->name,
                    device->name,
                    device->model,
                    device->ver);
                break;
            case HA_REG_SWITCH:
                snprintf(config, sizeof(config)-1, "{"
                    " \"name\": \"%s\","
                    " \"unique_id\": \"%s\","
                    " \"state_topic\": \"%s/%s/state\","
                    " \"command_topic\": \"%s/%s/cmd\","
                    " \"platform\": \"mqtt\","
                    " \"device\": {"
                        " \"identifiers\": [\"%s\"],"
                        " \"name\": \"%s\","
                        " \"manufacturer\": \"AtHome\","
                        " \"model\": \"%s\","
                        " \"sw_version\": \"%s\""
                        " }"
                    " }",
                    entity->name, 
                    entity->unique_id, 
                    cfg.mqtt_topic, entity->unique_id,
                    cfg.mqtt_topic, entity->unique_id,
                    device->name,
                    device->name,
                    device->model,
                    device->ver);
                break;
            case HA_REG_NUMBER:
                snprintf(config, sizeof(config)-1, "{"
                    " \"name\": \"%s\","
                    " \"unique_id\": \"%s\","
                    " \"state_topic\": \"%s/%s/state\","
                    " \"command_topic\": \"%s/%s/cmd\","
                    " \"platform\": \"mqtt\","
                    " \"device\": {"
                        " \"identifiers\": [\"%s\"],"
                        " \"name\": \"%s\","
                        " \"manufacturer\": \"AtHome\","
                        " \"model\": \"%s\","
                        " \"sw_version\": \"%s\""
                        " }"
                    " }",
                    entity->name, 
                    entity->unique_id, 
                    cfg.mqtt_topic, entity->unique_id,
                    cfg.mqtt_topic, entity->unique_id,
                    device->name,
                    device->name,
                    device->model,
                    device->ver);
                break;
        }
        error = mqtt_publish(mq(&client), topic, config, strlen(config), MQTT_PUBLISH_QOS_0|MQTT_PUBLISH_RETAIN);
        DBG("reg: %s: %s", topic, config);
        if(error != MQTT_OK)
        {
            DBG("error: %s", mqtt_error_str(error));
            break;
        }
    }
}


// ntohl(inet_addr(uplink_ip))
int open_uplink_socket()
{
    long fl;
    int res, err, sd = -1;
    struct sockaddr_in addr;

    do
    {
        // DBG("Open uplink");
        client.status = SOC_NONE;
        if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            err = errno;
            DBG("Failed to create uplink socket: %d", err);
            break;
        }

        /* set up destination address */
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(cfg.uplink_ip);
        addr.sin_port = htons(cfg.uplink_port);

        // Set non-blocking
        if((fl = fcntl(sd, F_GETFL, NULL)) < 0)
        {
            err = errno;
            DBG("Error fcntl(..., F_GETFL) (%s)", strerror(err));
            close(sd);
            sd = -1;
            break;
        }
        fl |= O_NONBLOCK;
        if(fcntl(sd, F_SETFL, fl) < 0)
        {
            err = errno;
            DBG("Error fcntl(..., F_SETFL) (%s)", strerror(err));
            close(sd);
            sd = -1;
            break;
        }

        if((res = connect(sd, (const struct sockaddr *)&addr, sizeof(addr))) < 0)
        {
            err = errno;
            if(err != EINPROGRESS)
            {
                DBG("Connect failed to %s: %d", inet_ntoa(addr.sin_addr), err);
                close(sd);
                sd = -1;
                break;
            }
            client.status = SOC_CONNECTING;
            client.addr = addr;
            client.addr_len = sizeof(addr);
            poll_fds.fds[client.n_fd].fd = sd;
            poll_fds.fds[client.n_fd].events = POLLOUT|POLLWRBAND;

            DBG("Delayed connection initiated %d", sd);
        }
    } while(0);

    if(sd > 0)
    {
        client.sd = sd;
    }
    return sd;
}

void uplink_socket_connected()
{
    long fl;
    int sd = client.sd;
    int err, valopt;
    socklen_t lon = sizeof(valopt);
    char topic[100];

    do
    {
        // DBG("Connected uplink");
        if(getsockopt(sd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
        {
            err = errno;
            DBG("Error in getsockopt() - %s", strerror(err));
            close(sd);
            client.sd = -1;
            client.status = SOC_NONE;
            sd = -1;
            break;
        }
        // Check the value returned...
        if(valopt)
        {
            DBG("Error in delayed connection() %d - %s", valopt, strerror(valopt));
            close(sd);
            client.sd = -1;
            client.status = SOC_NONE;
            sd = -1;
            break;
        }
        client.status = SOC_CONNECTED;
        poll_fds.fds[client.n_fd].events = POLLIN|POLLPRI|POLLRDHUP;

        DBG("MQTT: Connected %d", sd);
        mqtt_reinit(mq(&client), sd, client.sendbuf, sizeof(client.sendbuf), client.recvbuf, sizeof(client.recvbuf));

        /* Ensure we have a clean session */
        uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
        /* Send connection request to the broker. */
        mqtt_connect(mq(&client), cfg.client_name, NULL, NULL, 0, NULL, NULL, connect_flags, 300);

        /* check that we don't have any errors */
        if(client.client.error != MQTT_OK)
        {
            DBG("error: %s", mqtt_error_str(client.client.error));
            close(sd);
            client.sd = -1;
            client.status = SOC_NONE;
            sd = -1;
            break;
        }

        /* subscribe */
        snprintf(topic, sizeof(topic)-1, "%s/+/cmd", cfg.mqtt_topic);
        mqtt_subscribe(mq(&client), topic, 0);
    } while(0);
}

void handle_mqtt()
{
    enum MQTTErrors err;
    int n = client.n_fd;
    int revents = (n > 0) ? (poll_fds.fds[n].revents) : 0;
    
    // DBG("Handle mqtt");

    if(revents)
    {
        poll_fds.fds[n].revents = 0;
        
        switch(client.status)
        {
            case SOC_CONNECTING:
            {
                uplink_socket_connected(client.sd);
                break;
            }    
            case SOC_CONNECTED:
            {
                if(revents & POLLRDHUP)
                {
                    client.status = SOC_DISCONNECTED;
                }
                break;
            }
            case SOC_TIMEOUT:
            {
                uint64_t exp;
                // Reconnect timer fired
                read(client.sd, &exp, sizeof(exp));
                close(client.sd);
                client.sd = -1;
                poll_fds.fds[n].fd = -1;
                poll_fds.fds[n].events = 0;
                open_uplink_socket();
                break;
            }    
        }
    }
    if(client.status == SOC_CONNECTED)
    {
        /* Send HA registration requests */
        send_registry();

        err = mqtt_sync(mq(&client));
        if(err != MQTT_OK)
        {
            DBG("Error: %d: %s", mq(&client)->error, mqtt_error_str(mq(&client)->error));
            mqtt_disconnect(mq(&client));
            client.status = SOC_DISCONNECTED;
        }
    }
    if(client.status == SOC_DISCONNECTED)
    {
        DBG("MQTT: Reconnecting...");
        close(client.sd);
        client.sd = -1;
        poll_fds.fds[n].fd = -1;
        poll_fds.fds[n].events = 0;
        open_uplink_socket();
    }
    if(client.status == SOC_NONE && client.sd == -1)
    {
        struct itimerspec tmr_value;
        
        DBG("Setting timeout ...");
        
        client.status = SOC_TIMEOUT;
        tmr_value.it_value.tv_sec = 3;
        tmr_value.it_value.tv_nsec = 0;
        tmr_value.it_interval.tv_sec = 0;
        tmr_value.it_interval.tv_nsec = 0;

        // Start reconnect timeout
        client.sd = timerfd_create(CLOCK_MONOTONIC, 0);
        timerfd_settime(client.sd, 0, &tmr_value, NULL);
        poll_fds.fds[n].fd = client.sd;
        poll_fds.fds[n].events = POLLIN;
    }
}


void init_uplink()
{
    registry.n = 0;

    client.status = SOC_NONE;
    client.sd = -1;
    client.n_fd = -1;
    
    memset(&poll_fds, sizeof(poll_fds), 0);
    mqtt_init(mq(&client), -1, client.sendbuf, sizeof(client.sendbuf), client.recvbuf, sizeof(client.recvbuf), mqtt_callback);
    
    
}

void setup_uplink_poll()
{
    int n = poll_fds.n++;
    client.n_fd = n;
    poll_fds.fds[n].fd = -1;
    poll_fds.fds[n].events = 0;
    poll_fds.fds[n].revents = 0;

    open_uplink_socket();
}

void send_mqtt(const char* event, char* value)
{
    char topic[100];
    
    DBG("pub: %s %s", event, value);
    snprintf(topic, sizeof(topic)-1, "%s/%s", cfg.mqtt_topic, event);
    mqtt_publish(mq(&client), topic, value, strlen(value), MQTT_PUBLISH_QOS_0);
}

void mqtt_send_status(ha_entity_t* entity, char* status)
{
    char topic[100];

    snprintf(topic, sizeof(topic), "%s/%s/state", cfg.mqtt_topic, entity->unique_id);
    DBG("pub: %s: %s", topic, status);
    mqtt_publish(mq(&client), topic, status, strlen(status), MQTT_PUBLISH_QOS_0);
}

void mqtt_pin_status(ha_entity_t* entity, char* status)
{
    char topic[100];

    snprintf(topic, sizeof(topic), "%s/%s/state", cfg.mqtt_topic, entity->unique_id);
    DBG("pub: %s: %s", topic, status);
    mqtt_publish(mq(&client), topic, status, strlen(status), MQTT_PUBLISH_QOS_0|MQTT_PUBLISH_RETAIN);
}

void set_uplink_filter(const char* filter, msgfn_t cb, int param)
{
    int n = client.n_ft;
    
    if(n == MAX_FILTERS)
    {
        DBG("Error: Max filters reached!");
        return;
    }
    
    client.n_ft++;
    client.ft[n].str = filter;
    client.ft[n].cb = cb;
    client.ft[n].param = param;
}

/* All component registration should be done on INIT stage */
void ha_register_sensor(ha_device_t* device, ha_entity_t* entity)
{
    int i = registry.n;

    // Check for overflow
    if(i < MAX_HA_ENTITIES)
    {
        registry.type[i] = HA_REG_SENSOR;
        registry.dev[i] = device;
        registry.ent[i] = entity;
        registry.n++;
    }
}

void ha_register_binary(ha_device_t* device, ha_entity_t* entity)
{
    int i = registry.n;

    // Check for overflow
    if(i < MAX_HA_ENTITIES)
    {
        registry.type[i] = HA_REG_BINARY;
        registry.dev[i] = device;
        registry.ent[i] = entity;
        registry.n++;
    }
}

void ha_register_switch(ha_device_t* device, ha_entity_t* entity)
{
    int i = registry.n;

    // Check for overflow
    if(i < MAX_HA_ENTITIES)
    {
        registry.type[i] = HA_REG_SWITCH;
        registry.dev[i] = device;
        registry.ent[i] = entity;
        registry.n++;
    }
}

void ha_register_number(ha_device_t* device, ha_entity_t* entity)
{
    int i = registry.n;

    // Check for overflow
    if(i < MAX_HA_ENTITIES)
    {
        registry.type[i] = HA_REG_NUMBER;
        registry.dev[i] = device;
        registry.ent[i] = entity;
        registry.n++;
    }
}
