#ifndef __AVR__

#include "command.h"

#else /* __AVR__ */

#include <inttypes.h>
#include <string.h> // for memcpy

#include "command.h"
#include "config.h"
#include "ds1820.h"
#include "adc.h"
#include "pwm.h"

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

static uchar cmd_read_sensor(uchar len, uchar* data)
{
    uchar i;
    uchar n = 0;
    ushort val;
    
    switch(data[1])
    {
    case 0:
        data[1] = 0x02; /* Not Implemented yet */
        n = 2;
        break;
    case 1:
        // Read ADC In [0-F]
        val = adc_read(data[2] & 0x0F);
	    data[1] = 0x00; /* OK */
        data[2] = (val >> 8) & 0x00FF;
        data[3] = val & 0x00FF;
        n = 4;
        break;
    case 2:
        // Read PWM [0-1]
        val = pwm_read(data[2] & 0x01);
	    data[1] = 0x00; /* OK */
        data[2] = (val >> 8) & 0x00FF;
        data[3] = val & 0x00FF;
        n = 4;
        break;
    case 3:
    {
        // Read Temp sensors [0-7]
        i = data[2] & 0x07;
   	    uchar ls = ds1820probes[i].lasttemp[0];
	    uchar ms = ds1820probes[i].lasttemp[1];
	    data[1] = 0x00; /* OK */
        data[2] = ((ms << 4) & 0xF0) | ((ls >> 4) & 0x0F);
        data[3] = (ls << 4) & 0xF0;
        data[4] = ds1820probes[i].flags;
        memcpy(&data[5], ds1820probes[i].serial, 6);
        n = 11;
        break;
    }
    default:
        data[1] = 0x02; /* Not Implemented */
        n = 2;
        break;
    }
    return n;        
}

static uchar cmd_set_actuator(uchar len, uchar* data)
{
    uchar n = 0;
    ushort val;
    
    switch(data[1])
    {
    case 0:
        data[1] = 0x02; /* Not Implemented yet */
        n = 2;
        break;
    case 1:
        // Set ADC options
        data[1] = 0x02; /* Not Implemented yet */
        n = 2;
        break;
    case 2:
        // Set PWM [0-1]
        val = ((ushort)(data[2]) << 8) | data[3];
        pwm_set(data[2], val);
        data[1] = 0x00; /* OK */
        n = 2;
        break;
    case 3:
    {
        // Set Temp sensor options
        if(data[2] == 0) // Rescan sensors
        {
            ds1820count = 0;
            data[1] = 0x00; /* OK */
        }
        else
        {
            data[1] = 0x02; /* Not Implemented yet */
        }
        n = 2;
        break;
    }
    default:
        data[1] = 0x02; /* Not Implemented */
        n = 2;
        break;
    }

    return n;        
}

packet_t* cmd_proc(packet_t* pkt)
{
    uchar len = 0;
        
    pkt->to = pkt->from;
    pkt->from = cfg_node_id();
    pkt->via = 0;
    
    if(pkt->data[0] == 0x02 && pkt->len > 1) len = cmd_read_sensor(pkt->len, pkt->data);
    if(pkt->data[0] == 0x04 && pkt->len > 1) len = cmd_set_actuator(pkt->len, pkt->data);
    
    if(len)
    {
        pkt->data[0] = 0x01;
        return pkt;
    }
    return NULL;
}


#endif /* __AVR__ */
