#ifndef __AVR__

#else /* __AVR__ */

// #define KISS
#define F_CPU  12000000UL 

#include <avr/io.h>
#include "ds1wire.h"
#include "ds1820.h"

struct probe ds1820probes[DS1820_MAXPROBES];

void ds1820init(void) {
  ds1wire_init(C, 0);
  ds1wire_reset(C, 0);
}

void ds1820killbus(void) {
  ds1wire_pulldown(C, 0);
}

static void ds1820write(uint8_t val) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    if (val & 0x01) {
      ds1wire_send1(C, 0);
    } else {
      ds1wire_send0(C, 0);
    }
    val >>= 1;
  }
}

static uint8_t ds1820read(void) {
  uint8_t i;
  uint8_t res = 0;
  for (i = 0; i < 8; i++) {
    res |= ((ds1wire_read(C, 0)) << i);
  }
  return res;
}

void ds1820queryprobe(uint8_t probenum)
{
  uint8_t i; uint8_t crc = 0;
  ds1wire_reset(C, 0);
#ifdef KISS
  ds1820write(DS1820_CMD_SKIPROM); /* Skip ROM */
#else
  ds1820write(DS1820_CMD_MATCHROM); /* Match ROM */
  /* Send 64 bit serial */
  ds1820write(ds1820probes[probenum].family);
  crc = ds1wire_calccrc8(crc, ds1820probes[probenum].family);
  for (i = 0; i < 6; i++) {
    ds1820write(ds1820probes[probenum].serial[i]);
    crc = ds1wire_calccrc8(crc, ds1820probes[probenum].serial[i]);
  }
  ds1820write(crc);
#endif /* KISS */
  /* Issue 'start temperature conversion' */
  ds1820write(DS1820_CMD_CONVERTT);
  if (ds1820probes[probenum].flags & DS1820FLAG_PARASITE) {
    /* Provide parasite power */
    ds1wire_parasitepoweron(C, 0);
  }
}

/* Returns 1 on successful update, 0 otherwise */
uint8_t ds1820updateprobe(uint8_t probenum)
{
  uint8_t i; uint8_t crc = 0; uint8_t t1 = 0; uint8_t t2 = 0;
  if (ds1820probes[probenum].flags & DS1820FLAG_PARASITE) {
    /* No longer provide parasite power */
    ds1wire_parasitepoweroff(C, 0);
  }
  ds1wire_reset(C, 0);
#ifdef KISS
  ds1820write(DS1820_CMD_SKIPROM); /* Skip ROM */
#else
  ds1820write(DS1820_CMD_MATCHROM); /* Match ROM */
  /* Send 64 bit serial */
  ds1820write(ds1820probes[probenum].family);
  crc = ds1wire_calccrc8(crc, ds1820probes[probenum].family);
  for (i = 0; i < 6; i++) {
    ds1820write(ds1820probes[probenum].serial[i]);
    crc = ds1wire_calccrc8(crc, ds1820probes[probenum].serial[i]);
  }
  ds1820write(crc);
#endif /* KISS */
  /* Read Scratchpad */
  ds1820write(DS1820_CMD_READSCRPAD);
  crc = 0;
  for (i = 0; i < 9; i++) {
    uint8_t v = ds1820read();
    if (i == 0) { t1 = v; }
    if (i == 1) { t2 = v; }
    crc = ds1wire_calccrc8(crc, v);
  }
  /* For parasite powered probes, reading 0x50 0x05 (85.0 deg) usually
   * means the probe has lost power during temperature conversion and
   * reset (85.0 is the poweron value of the temperature scratchpad).
   * So we filter the value 85.0 for parasite powered probes. This means
   * you can never use parasite powered probes for temperatures in this
   * range. */
  if ((crc == 0)
   && (((ds1820probes[probenum].flags & DS1820FLAG_PARASITE) == 0)
    || (t1 != 0x50) || (t2 != 0x05))) {
    ds1820probes[probenum].lastts = 0; // TODO: gettime();
    ds1820probes[probenum].lasttemp[0] = t1;
    ds1820probes[probenum].lasttemp[1] = t2;
    return 1;
  } else {
    return 0;
  }
}

/* This is going to be a tough one:
 * Scan the bus to find all probes.
 */
uint8_t ds1820scan(void) {
#ifdef KISS
  ds1wire_reset(C, 0);
  ds1820probes[0].flags |= DS1820FLAG_SLOTINUSE;
  ds1820write(DS1820_CMD_SKIPROM); /* Skip ROM */
  ds1820write(DS1820_CMD_READPOWER); /* Read power supply */
  if (ds1wire_read(C, 0) == 0) { /* Parasite powered probes return 0 */
    ds1820probes[0].flags |= DS1820FLAG_PARASITE;
  } else {
    ds1820probes[0].flags &= ~DS1820FLAG_PARASITE;
  }
  return 1;
#else /* KISS */
  uint8_t lastserialfound[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  int8_t lastcolwith0 = -1;
  int8_t prevcolwith0;
  int8_t i;
  uint8_t j;
  uint8_t curprobe = 0;
  for (i = 0; i < DS1820_MAXPROBES; i++) { /* Clear list of probes */
    ds1820probes[i].flags = 0;
  }
  do {
    ds1wire_reset(C, 0);
    prevcolwith0 = lastcolwith0;
    lastcolwith0 = -1;
    /* Send scan command */
    ds1820write(DS1820_CMD_SEARCHROM);
    for (i = 0; i < 64; i++) {
      uint8_t val1 = ds1wire_read(C, 0);
      uint8_t val2 = ds1wire_read(C, 0);
      if (val1 == val2) { /* Collission */
        if (val1 == 1) { /* and thus val2 is 1 too */
          /* Nothing matched on the bus! This is actually pretty fatal! */
          /* Try to get out alive. Send all 0. */
          ds1wire_send0(C, 0); lastcolwith0 = -1;
        } else { /* Both 0 */
          /* Was that where we stopped last time? */
          if (prevcolwith0 == i) { /* Send a 1 this time */
            ds1wire_send1(C, 0);
            lastserialfound[i >> 3] |= _BV(i & 0x07);
          } else {
            if (i < prevcolwith0) { /* We are before the position of the */
              /* previous collission. Select the same as in the last iteration */
              if (lastserialfound[i >> 3] & _BV(i & 0x07)) {
                ds1wire_send1(C, 0);
              } else {
                lastcolwith0 = i;
                ds1wire_send0(C, 0);
              }
            } else { /* New collission */
              lastcolwith0 = i;
              ds1wire_send0(C, 0);
              lastserialfound[i >> 3] &= (uint8_t)~_BV(i & 0x07);
            }
          }
        }
      } else { /* val1 != val2 */
        if (val1 == 0) { /* Was a 0, so select that */
          ds1wire_send0(C, 0);
          lastserialfound[i >> 3] &= (uint8_t)~_BV(i & 0x07);
        } else { /* Was a 1 */
          ds1wire_send1(C, 0);
          lastserialfound[i >> 3] |= _BV(i & 0x07);
        }
      }
    }
    /* lastserialfound now contains the serial of the last probe we found,
     * AND that probe is selected. */
    /* Check CRC of serial number. */
    j = 0; /* Used as CRC here */
    for (i = 0; i < 8; i++) {
      j = ds1wire_calccrc8(j, lastserialfound[i]);
    }
    if (j == 0) { /* CRC of serial number OK! */
      ds1820probes[curprobe].flags |= DS1820FLAG_SLOTINUSE;
      ds1820probes[curprobe].family = lastserialfound[0];
      for (j = 0; j < 6; j++) {
        ds1820probes[curprobe].serial[j] = lastserialfound[j+1];
      }
      ds1820probes[curprobe].lastts = 0;
      ds1820probes[curprobe].lasttemp[0] = 0;
      ds1820probes[curprobe].lasttemp[1] = 0;
      /* Find out if the probe is parasite powered or not. */
      ds1820write(DS1820_CMD_READPOWER); /* Read power supply */
      if (ds1wire_read(C, 0) == 0) { /* Parasite powered probes return 0 */
        ds1820probes[curprobe].flags |= DS1820FLAG_PARASITE;
      } else {
        ds1820probes[curprobe].flags &= ~DS1820FLAG_PARASITE;
      }
      curprobe++;
      if (curprobe >= DS1820_MAXPROBES) { /* No space left to learn more probes */
        break;
      }
    }
  } while (lastcolwith0 >= 0);
  return curprobe;
#endif /* KISS */
}

#endif /* __AVR__ */

