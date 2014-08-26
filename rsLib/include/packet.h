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
    uchar  seq; // req/resp must match this field
    uchar  len;
    uchar  data[0];
} packet_t;
#pragma pack(pop)

// TODO: Define flags field

ushort packet_crc(packet_t* pkt);

#endif /* _PACKET_H_ */
