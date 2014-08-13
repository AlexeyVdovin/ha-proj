#ifndef _PACKET_H_
#define _PACKET_H_

#include "defines.h"

#define MAX_DATA_LEN    16
#define DATA_ID1        0xAA
#define DATA_ID2        0xC4


#pragma pack(push, 1)
typedef struct
{
    uchar  id[2];
    uchar  to;
    uchar  from;
    uchar  flags;
    uchar  len;
    uchar  data[0];
} packet_t;
#pragma pack(pop)

void packet_init();

packet_t* rx_packet();
void tx_packet(packet_t* pkt);


#endif /* _PACKET_H_ */
