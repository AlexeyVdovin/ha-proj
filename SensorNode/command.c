#ifndef __AVR__

#include "command.h"

#else /* __AVR__ */

#include <inttypes.h>

#include "command.h"

/*
    Sensors:
    00 - System staus
    01 - ADC input
    02 - PWM channel
    03 - Temp sensor
    
    Actuators:
    80 - System operations
    81 - ADC options
    82 - PWM channel
    83 - Temp sensor options

*/

packet_t* cmd_proc(packet_t* pkt)
{
    uchar from = pkt->from;
    pkt->from = pkt->to;
    pkt->to = from;
    pkt->via = 0;

    return pkt;
}


#endif /* __AVR__ */
