/* $Id: ds1820.h,v 1.3 2013-07-21 07:45:01 simimeie Exp $
 * USB interface for ds1820
 * This file handles everything that has to do with the temperature probes.
 * (C) Michael "Fox" Meier 2009
 * There are two modes: KISS (keep it simple stupid), activated when you
 * define KISS, can only ever support one probe on the bus. When KISS is
 * not defined, multiple probes are supported up to a compile time maximum,
 * and the bus is searched for probes.
 */

#ifndef __DS1820_H__
#define __DS1820_H__

#include <inttypes.h>

/* Number of probes supported */
#define DS1820_MAXPROBES  4

#ifdef KISS  /* We can only ever handle one probe in this mode, so override define */
#undef DS1820_MAXPROBES
#define DS1820_MAXPROBES 1
#endif

#define DS1820FLAG_SLOTINUSE  0x01      /* 1 if this slot is in use */
#define DS1820FLAG_PARASITE   0x02      /* 1 if the device is parasite powered */

#define DS1820_CMD_SEARCHROM	0xF0
#define DS1820_CMD_SKIPROM	0xCC
#define DS1820_CMD_MATCHROM	0x55
#define DS1820_CMD_READROM	0x33
#define DS1820_CMD_CONVERTT	0x44
#define DS1820_CMD_READSCRPAD	0xBE
#define DS1820_CMD_READPOWER	0xB4

struct probe {
  uint8_t family;
  uint8_t serial[6];
  uint8_t flags;
  uint8_t lasttemp[2]; /* Temperature we read last */
  uint32_t lastts;  /* Timestamp when we last read it */
};

extern struct probe ds1820probes[DS1820_MAXPROBES];

/* Inits ds1820 code and bus */
void ds1820init(void);

/* Actively pull down the bus. For hard resetting probes that have gone
 * bonkers. */
void ds1820killbus(void);

/* Scan the bus to find all probes. Populates the ds1820probes array. */
void ds1820scan(void);

/* Tells one probe to do temperature conversion. Since that takes
 * about 1 second, you will need to wait that long before
 * calling ds1820updateprobe.
 * the number is the index number for the ds1820probes array. */
void ds1820queryprobe(uint8_t probenum);

/* Reads the answer from the probe queried with ds1820queryprobe
 * before and updates its data.
 * the number is the index number for the ds1820probes array.
 * Returns 1 on successful update, 0 otherwise */
uint8_t ds1820updateprobe(uint8_t probenum);

#endif /* __DS1820_H__ */
