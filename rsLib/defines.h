#ifndef _DEFINES_H_
#define _DEFINES_H_

#ifndef __AVR__
    #define _DEV_SIM_
#endif

#ifdef _DEV_SIM_
    #define _PACKET_TRANSPORT_UDP_
    #define UDP_PORT            15432
#else
    #define _PACKET_TRANSPORT_SIO_
#endif

#ifndef uchar
#define uchar   unsigned char
#endif

#ifndef ushort
#define ushort  unsigned short
#endif

#ifndef uint
#define uint    unsigned int
#endif

#ifndef ulong
#define ulong   unsigned long
#endif

#ifndef NULL
#define NULL    ((void*)0)
#endif

#define SYSTEM_CLOCK              12000000L

/* for <util/setbaud.h> */
#define F_CPU                     SYSTEM_CLOCK
#define BAUD                      57600


#endif
