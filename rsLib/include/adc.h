#ifndef _ADC_H_
#define _ADC_H_

#include "defines.h"

#define ADC_IN1         4
#define ADC_IN2         5
#define ADC_VREF_TYPE   0xC0

void adc_init();

ushort adc_read(uchar ch);

#endif /* _ADC_H_ */
