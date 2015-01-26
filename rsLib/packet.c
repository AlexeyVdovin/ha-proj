#ifndef __AVR__
#include "packet.h"

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

#else /* __AVR__ */
#include <util/crc16.h>

#include "packet.h"

ushort packet_crc(packet_t* pkt)
{
    register ushort crc = 0;
    uchar* p = (uchar*)pkt;
    uchar i;
    
    for(i = 0; i < pkt->len + sizeof(packet_t); ++i) crc = _crc_xmodem_update(crc, p[i]);

    return crc;
}
#endif /* __AVR__ */
