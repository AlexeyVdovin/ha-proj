/* $Id: ds1wire.h,v 1.2 2010/03/23 07:57:04 simimeie Exp $
 * Basic operations for dallas / maxim 1wire bus
 * (C) Michael "Fox" Meier 2009
 */

#ifndef __DS1WIRE_H__
#define __DS1WIRE_H__

// #include <util/delay.h>
// #define _delay_ms(a)
//#define _delay_us(a) delay_us(a)
//#define _delay_ms(a) delay_ms(a)

#include "defines.h"
#include <util/delay_basic.h>

static void delay_us(unsigned short time_us) {
	_delay_loop_2((unsigned short)(F_CPU/4000000UL)*time_us);
} 
// #define delay_ms(time_ms) delay_us(time_ms*1000)

/* This macro is redundant after powerup, as all bits zero is the default
 * state after powerup. */
#define ds1wire_init(port, bit) {\
  DDR##port &= (uint8_t)~(_BV(bit));  /* Do not actively drive the bus */ \
  PORT##port &= (uint8_t)~(_BV(bit)); /* Always output a 0, so we just need to change */ \
                                      /* the DDR register to pull the bus low. */ \
}

/* Reset the 1 wire bus. Works by driving it low for > 480 us.
 * we do 500 to make sure.
 * We then read the presence pulse 60 us after releasing the bus,
 * and wait for another 420+1 us for the bus to become free.
 * Returns 1 if there was a presence pulse, and 0 otherwise.
 * takes about 981 usec to execute.
 */
#define ds1wire_reset(port, bit) \
  __ds1wire_reset(&DDR##port, &PIN##port, _BV(bit))

static inline uint8_t __ds1wire_reset(volatile uint8_t * ddr, volatile uint8_t * pin, uint8_t bv) {
  uint8_t ret;
  *ddr |= bv; /* drive the bus low */
  delay_us(500);
  *ddr &= (uint8_t)~bv; /* release the bus */
  delay_us(60);
  ret = ((*pin & bv) == 0);
  delay_us(421);
  return ret;
}

/* Actively pull down the 1 wire bus. For hard resetting parasite powered
 * probes that have gone bonkers. */
#define ds1wire_pulldown(port, bit) {\
  PORT##port &= (uint8_t)~(_BV(bit)); /* Output 0 (sucking away the current from the pullup) */ \
  DDR##port |= _BV(bit);  /* Actively drive the bus */ \
}

/* Read a bit from the 1 wire bus. takes 61 usec to execute. */
#define ds1wire_read(port, bit) \
  __ds1wire_read(&DDR##port, &PIN##port, _BV(bit))

static inline uint8_t __ds1wire_read(volatile uint8_t * ddr, volatile uint8_t * pin, uint8_t bv) {
  uint8_t ret;
  *ddr |= bv; /* drive the bus low */
  delay_us(5); /* everything > 1 us should suffice, more to be sure. */
  *ddr &= (uint8_t)~bv; /* release the bus */
  delay_us(6); /* Wait for the probe to pull the bus */
  ret = ((*pin & bv) != 0);
  delay_us(50); /* after that the bus is free again */
  return ret;
}

/* Sends a 0 bit to the 1 wire bus. takes 61 usec to execute. */
#define ds1wire_send0(port, bit) {\
  DDR##port |= _BV(bit); /* pull low */ \
  delay_us(60); /* worst case timing */ \
  DDR##port &= (uint8_t)~_BV(bit); /* release */ \
  delay_us(1); /* bus is free again after that */ \
  }

/* Sends a 1 bit to the 1 wire bus. takes 61 usec to execute. */
#define ds1wire_send1(port, bit) {\
  DDR##port |= _BV(bit); /* pull low */ \
  delay_us(10); /* everything > 1 us should suffice, try to be close to 15 us. */ \
  DDR##port &= (uint8_t)~_BV(bit); /* release */ \
  delay_us(51); /* bus is free again after that */ \
}

/* Enable power for parasite powered probes. Warning: You must
 * not forget to call parasitepoweroff after some time! */
#define ds1wire_parasitepoweron(port, bit) { \
  PORT##port |= _BV(bit); \
  DDR##port |= _BV(bit); \
}

/* Disable power for parasite powered probes. */
#define ds1wire_parasitepoweroff(port, bit) \
  ds1wire_init(port, bit)

static inline uint8_t ds1wire_calccrc8(uint8_t shiftreg, uint8_t data)
{
  uint8_t i;
  for (i = 0; i < 8; i++) {
    /* XOR LSB of shiftreg with LSB of data */
    uint8_t feedback = (shiftreg & 0x01) ^ (data & 0x01);
    shiftreg >>= 1; /* first position in shiftreg now is 0 - important for below! */
    if (feedback == 1) {
      shiftreg = shiftreg ^ 0x8C; /* binary 10001100 */
    }
    data >>= 1;
  }
  return shiftreg; 
}

#endif /* __DS1WIRE_H__ */
