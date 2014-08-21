#ifndef __AVR__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>

#include "packet.h"

#ifdef _PACKET_TRANSPORT_SIO_
#include "sio.h"
#include "timer.h"
#endif

#ifdef _PACKET_TRANSPORT_UDP_
static int s = -1;
#endif

/* Calculating XMODEM CRC-16 in 'C'
   ================================
   Reference model for the translated code */

#define poly 0x1021

/* On entry, addr=>start of data
             num = length of data
             crc = incoming CRC     */
static unsigned short crc16(char *addr, int num, unsigned int crc)
{
    int i;

    for(; num>0; num--)                      /* Step through bytes in memory */
    {
        crc = crc ^ (*addr++ << 8);          /* Fetch byte from memory, XOR into CRC top byte*/
        for (i=0; i<8; i++)                  /* Prepare to rotate 8 bits */
        {
            crc = crc << 1;                  /* rotate */
            if (crc & 0x10000)               /* bit 15 was set (now bit 16)... */
                crc = (crc ^ poly) & 0xFFFF; /* XOR with XMODEM polynomic */
                                             /* and ensure CRC remains 16-bit value */
        }                                    /* Loop for 8 bits */
    }                                        /* Loop until num=0 */
    return (unsigned short)crc;              /* Return updated CRC */
}

/* Calc packet CRC */
ushort packet_crc(packet_t* pkt)
{
    uchar* p = (uchar*)pkt;
    ushort crc = crc16(p, sizeof(packet_t) + pkt->len, 0);
    return crc;
}

void packet_init()
{
#ifdef _PACKET_TRANSPORT_UDP_
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
#endif /* _PACKET_TRANSPORT_UDP_ */
}

static uchar rx_pkt[sizeof(packet_t)+MAX_DATA_LEN+2] = { 0 };
static uchar rx_pos = 0;
static ulong rx_time = 0;

packet_t* rx_packet()
{
    packet_t* pkt = (packet_t*)rx_pkt;
    
#ifdef _PACKET_TRANSPORT_UDP_
    size_t len = 0;
    ssize_t cb = recvfrom(s, rx_pkt, sizeof(rx_pkt), MSG_DONTWAIT, NULL, NULL);
    if(cb == -1)
    {
        if(errno != EAGAIN || errno != EWOULDBLOCK) printf("recvfrom() error: %d\n", errno);
    }
    else 
    {
        if(cb > sizeof(packet_t)) len = cb;
    }

    if(len && pkt->id[0] == DATA_ID1 && pkt->id[1] == DATA_ID2 && pkt->len <= MAX_DATA_LEN && pkt->len > 0)
    {
        ushort* crc = (ushort*)(pkt->data + pkt->len);
        if(*crc == packet_crc(pkt)) return pkt;
    }
#endif /* _PACKET_TRANSPORT_UDP_ */
#ifdef _PACKET_TRANSPORT_SIO_
    packet_t *pkt = (packet_t*)rx_pkt;

    // Check for timeout    
    if(rx_pos && rx_time && get_time() - rx_time > PACKET_RX_TIMEOUT) rx_pos = 0;

    while(sio_rxcount())
    {
        int c = sio_getchar();
        if(c < 0) break;
        uchar u = (uchar)(c & 0x00FF);
        
        if(rx_pos == 0)
        {
            if(u != DATA_ID1) continue;
            else rx_time = get_time();
        }
        if(rx_pos == 1 && u != DATA_ID2) { rx_pos = 0; continue; }
        if(rx_pos == 5 && u > MAX_DATA_LEN) { rx_pos = 0; continue; }
        if(rx_pos <= 5 || (rx_pos > 5 && rx_pos < sizeof(packet_t) + pkt->len + 2))
        { 
            rx_pkt[rx_pos++] = u;
        }
        if(rx_pos >= sizeof(packet_t) + pkt->len + 2)
        {
            rx_pos = 0;
            ushort* crc = (ushort*)(pkt->data + pkt->len);
            if(pkt->len > 0 && *crc == packet_crc(pkt)) return pkt;
            // printf("CRC missmatch!\n");
        }
    }
#endif /* _PACKET_TRANSPORT_SIO_ */
    return NULL;
}

void tx_packet(packet_t* pkt)
{
    ushort* crc = (ushort*)(pkt->data + pkt->len);
    *crc = packet_crc(pkt);
        
#ifdef _PACKET_TRANSPORT_UDP_
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = UDP_PORT;
    addr.sin_addr.s_addr = inet_addr("226.1.1.1");
    ssize_t cb = sendto(s, pkt, sizeof(packet_t)+pkt->len+2, SO_BROADCAST, (struct sockaddr*)&addr, sizeof(addr));
    if(cb == -1) printf("sendto() error: %d\n", errno);
#endif /* _PACKET_TRANSPORT_UDP_ */
#ifdef _PACKET_TRANSPORT_SIO_
    uchar i, *c = (uchar*)pkt;

    for(i = 0; i < pkt->len + sizeof(packet_t)+2; ++i) sio_putchar(c[i]);
#endif /* _PACKET_TRANSPORT_SIO_ */
}

#else /* __AVR__ */

#include <util/crc16.h>

#include "sio.h"
#include "timer.h"
#include "packet.h"

#define PACKET_RX_TIMEOUT ((1000000/(BAUD/10)*(sizeof(packet_t)+MAX_DATA_LEN+2)*2+5000)/10000)

static uchar rx_pkt[sizeof(packet_t)+MAX_DATA_LEN+2] = { 0 };
static uchar rx_pos = 0;
static ulong rx_time = 0;

ushort packet_crc(packet_t* pkt)
{
    ushort crc = 0;
    uchar* p = (uchar*)pkt;
    uchar i;
    
    for(i = 0; i < pkt->len + sizeof(packet_t); ++i) crc = _crc_xmodem_update(crc, p[i]);

    return crc;
}

void packet_init()
{
    rx_pos = 0;
}

packet_t* rx_packet()
{
    packet_t *pkt = (packet_t*)rx_pkt;

    // Check for timeout    
    if(rx_pos && rx_time && get_time() - rx_time > PACKET_RX_TIMEOUT) rx_pos = 0;

    while(sio_rxcount())
    {
        int c = sio_getchar();
        if(c < 0) break;
        uchar u = (uchar)(c & 0x00FF);
        
        if(rx_pos == 0)
        {
            if(u != DATA_ID1) continue;
            else rx_time = get_time();
        }
        if(rx_pos == 1 && u != DATA_ID2) { rx_pos = 0; continue; }
        if(rx_pos == 5 && u > MAX_DATA_LEN) { rx_pos = 0; continue; }
        if(rx_pos <= 5 || (rx_pos > 5 && rx_pos < sizeof(packet_t) + pkt->len + 2))
        { 
            rx_pkt[rx_pos++] = u;
        }
        if(rx_pos >= sizeof(packet_t) + pkt->len + 2)
        {
            rx_pos = 0;
            ushort* crc = (ushort*)(pkt->data + pkt->len);
            if(pkt->len > 0 && *crc == packet_crc(pkt)) return pkt;
            // printf("CRC missmatch!\n");
        }
    }
    return NULL;
}

void tx_packet(packet_t* pkt)
{
    uchar i, *c = (uchar*)pkt;
    ushort* crc = (ushort*)(pkt->data + pkt->len);
    *crc = packet_crc(pkt);
    for(i = 0; i < pkt->len + sizeof(packet_t)+2; ++i) sio_putchar(c[i]);
}


#endif /* __AVR__ */
