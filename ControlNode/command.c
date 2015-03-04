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
        // Read ADC In [0-1F]
        // 0 - Vin
        // 1 - Ch1 current sence
        // 2 - Ch0 Vout
        // 3 - Ch0 current sence
        // 4 - Ch1 Vout
        // 1E - 1.1V
        // 1F - 0V
        val = adc_read(data[2] & 0x1F);
	    data[1] = 0x00; /* OK */
        data[2] = (val >> 8) & 0x00FF;
        data[3] = val & 0x00FF;
        n = 4;
        break;
    case 2:
        // Read PWM [0-1]
        val = swpwm_read(data[2] & 0x01);
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
        data[5] = ds1820probes[i].family;
        memcpy(data+6, ds1820probes[i].serial, sizeof(ds1820probes[i].serial));
        n = 6 + sizeof(ds1820probes[i].serial);
        break;
    }
    default:
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
    case 0x80:
        data[1] = 0x02; /* Not Implemented yet */
        n = 2;
        break;
    case 0x81:
        // Set ADC options
        data[1] = 0x02; /* Not Implemented yet */
        n = 2;
        break;
    case 0x82:
        if(data[2] == 0 && len == 4)
        {
            swpwm_freq(data[3]);
            data[1] = 0x00; /* OK */
            n = 2;
        }
        else
        {
            // Set PWM [1-2]
            if(len == 5)
            {
                val = ((ushort)(data[3]) << 8) | data[4];
                swpwm_set(data[2], val);
                data[1] = 0x00; /* OK */
                n = 2;
            }
        }
        break;
    case 0x83:
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
    }
    else
    {
        pkt->data[0] = 0x01;
        pkt->data[1] = 0x02; /* Not Implemented */
        len = 2;
    }
    pkt->len = len;
    return pkt;
}


#endif /* __AVR__ */
