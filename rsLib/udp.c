#ifndef __AVR__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>

#include "timer.h"
#include "packet.h"

// Socket FD
static int s = -1;

static uchar udp_rx_data[sizeof(packet_t)+MAX_DATA_LEN+4] = { 0 };
static uchar udp_rx_pos   = 0;
static uchar udp_rx_count = 0;

static char tx_byte(uchar c) { }

static int rx_byte()
{
    return (udp_rx_data[udp_rx_pos++] & 0x00FF);
}

static uchar rx_count()
{
    return udp_rx_count - udp_rx_pos;
}

#define PACKET_RX_TIMEOUT (1)

#include "packetio.h"

void udp_init()
{
    struct in_addr localInterface;
    struct sockaddr_in groupSock;
    struct ip_mreq group;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_port = UDP_PORT;
    addr.sin_addr.s_addr = INADDR_ANY;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s == -1) { printf("socket() error: %d\n", errno); exit(1); }

    localInterface.s_addr = inet_addr("127.0.0.1");
    int err = setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface));
    if(err) { printf("setsockopt(IP_MULTICAST_IF) error: %d\n", errno); exit(1); }
    
    int val = 1;
    err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if(err) { printf("setsockopt(SO_REUSEADDR) error: %d\n", errno); exit(1); }
    
    err = bind(s, (struct sockaddr*)&addr, sizeof(addr));
    if(err) { printf("bind() error: %d\n", errno); exit(1); }
    
    /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
    /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
    /* called for each local interface over which the multicast */
    /* datagrams are to be received. */
    group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
    group.imr_interface.s_addr = inet_addr("127.0.0.1");
    err = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
    if(err) { printf("setsockopt(IP_ADD_MEMBERSHIP) error: %d\n", errno); exit(1); }
}

packet_t* udp_rx_packet()
{
    size_t len = 0;
    ssize_t cb = recvfrom(s, udp_rx_data, sizeof(udp_rx_data), MSG_DONTWAIT, NULL, NULL);
    if(cb == -1)
    {
        if(errno != EAGAIN || errno != EWOULDBLOCK) printf("recvfrom() error: %d\n", errno);
    }
    else if(cb > sizeof(packet_t))
    {
        udp_rx_pos = 0;
        udp_rx_count = cb;
        return rx_packet();
    }
    return NULL;
}

void udp_tx_packet(packet_t* pkt)
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = UDP_PORT;
    addr.sin_addr.s_addr = inet_addr("226.1.1.1");

    tx_packet(pkt);

    ssize_t cb = sendto(s, pkt, sizeof(packet_t)+pkt->len+2, SO_BROADCAST, (struct sockaddr*)&addr, sizeof(addr));
    if(cb == -1) printf("sendto() error: %d\n", errno);
}

#else  /* __AVR__ */

#endif /* __AVR__ */
