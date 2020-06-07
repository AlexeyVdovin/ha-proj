#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "config.h"
#include "uplink.h"

typedef struct
{
    struct mqtt_client client;
    uint8_t sendbuf[2048];
    uint8_t recvbuf[1024];
    struct sockaddr_in addr;
    socklen_t addr_len;
    t_soc_st  status;
    int       sd;
    int       n_fd;
} mqtt_t;


poll_t      poll_fds;
mqtt_t      client;

static inline struct mqtt_client* mq(mqtt_t* c) { return &c->client; }


static void process_mqtt(const char* topic, size_t topic_len, const char* message, size_t message_len)
{

}

static void mqtt_callback(void** unused, struct mqtt_response_publish *published)
{
    DBG("Received publish('%.*s'): %.*s", (int)published->topic_name_size, (const char*)published->topic_name, (int)published->application_message_size, (const char*)published->application_message);
    process_mqtt((const char*)published->topic_name, published->topic_name_size, (const char*)published->application_message, published->application_message_size);
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
            DBG("Error fcntl(..., F_GETFL) (%s)\n", strerror(err));
            close(sd);
            sd = -1;
            break;
        }
        fl |= O_NONBLOCK;
        if(fcntl(sd, F_SETFL, fl) < 0)
        {
            err = errno;
            DBG("Error fcntl(..., F_SETFL) (%s)\n", strerror(err));
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
            DBG("Error in getsockopt() - %s\n", strerror(err));
            close(sd);
            client.sd = -1;
            client.status = SOC_NONE;
            sd = -1;
            break;
        }
        // Check the value returned...
        if(valopt)
        {
            // DBG("Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
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
        }

        /* subscribe */
        snprintf(topic, sizeof(topic)-1, "%s/cmd", cfg.mqtt_topic);
        mqtt_subscribe(mq(&client), topic, 0);

    } while(0);
}

void handle_mqtt()
{
    int n = client.n_fd;
    int revents = (n > 0) ? (poll_fds.fds[n].revents) : 0;
    
    // DBG("Handle mqtt");

    if(revents)
    {
        poll_fds.fds[n].revents = 0;
        if(client.status == SOC_CONNECTING)
        {
            uplink_socket_connected(client.sd);
        }
        else if(client.status == SOC_CONNECTED)
        {
            if(revents & POLLRDHUP)
            {
                client.status = SOC_DISCONNECTED;
                DBG("MQTT: Reconnecting...");
                close(client.sd);
                client.sd = -1;
                poll_fds.fds[n].fd = -1;
                poll_fds.fds[n].events = 0;
                open_uplink_socket();
            }
        }
    }
    mqtt_sync(mq(&client));
}


void init_uplink()
{
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

void send_mqtt(char* event, char* value)
{
    char topic[100];
    
    DBG("pub: %s %s", event, value);
    snprintf(topic, sizeof(topic)-1, "%s/%s", cfg.mqtt_topic, event);
    mqtt_publish(mq(&client), topic, value, strlen(value), MQTT_PUBLISH_QOS_0);
}

