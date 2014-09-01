#ifndef __AVR__

#include "command.h"

#else /* __AVR__ */

#include <inttypes.h>

#include "command.h"
#include "config.h"
#include "ds1820.h"
#include "adc.h"

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

static uchar cmd_read_sensor(uchar* data)
{
    uchar i;
    uchar len = 0;
    ushort val;
    
    switch(data[1])
    {
    case 0:
        data[1] = 0x02;
        len = 2;
        break;
    case 1:
        val = adc_read(data[2]);
        data[1] = (uchar)(val & 0x00FF);
        data[2] = (uchar)((val >> 8) & 0x00FF);
        len = 3;
        break;
    case 2:
        /* TBD */
        data[1] = 0x02;
        len = 2;
        break;
    case 3:
        i = data[2];
        data[1] = ds1820probes[i].lasttemp[0];
        data[2] = ds1820probes[i].lasttemp[1];
        len = 3;
        break;
    default:
        data[1] = 0x01; /* FAIL */
        len = 2;
        break;
    }
    return len;        
}

packet_t* cmd_proc(packet_t* pkt)
{
    uchar len = 0;
    
    // Not mine packet. This check maybe need to be moved to packetio.h ???
    if(pkt->to != 0 && pkt->to != cfg_node_id()) return NULL;
    
    pkt->to = pkt->from;
    pkt->from = cfg_node_id();
    pkt->via = 0;
    
    if(pkt->data[0] == 0x02 && pkt->len > 1) len = cmd_read_sensor(pkt->data);
    
    if(len)
    {
        pkt->data[0] = 0x01;
        return pkt;
    }
    return NULL;
}


#endif /* __AVR__ */
