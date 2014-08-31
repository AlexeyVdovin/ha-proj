#ifndef __AVR__

#include "command.h"

#else /* __AVR__ */

#include <inttypes.h>

#include "command.h"

packet_t* cmd_proc(packet_t* pkt)
{
    uchar from = pkt->from;
    pkt->from = pkt->to;
    pkt->to = from;
    pkt->via = 0;

    return pkt;
}


#endif /* __AVR__ */
