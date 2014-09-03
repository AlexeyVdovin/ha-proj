#ifndef __AVR__

#include "adc.h"

void adc_init()
{
    
}

ushort adc_read(uchar ch)
{
    return 0;
}

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "adc.h"
#include "timer.h"
#include "ds1wire.h" // For delay_us()

/* ADC Channels:
 00  ADC0
 01  ADC1
 02  ADC2
 03  ADC3
 04  ADC4
 05  ADC5
 06  ADC6 - In1
 07  ADC7 - In2
 0E  1.30V (VBG)
 0F  0V (GND)
*/

ISR(ADC_vect)
{

}

void adc_init()
{
    // ADC Clock frequency: 93.750 kHz
    // ADC Voltage Reference: Int., cap. on AREF
    ADMUX=(ADC_VREF_TYPE & 0xC0)|(ADC_IN1&0x0F);
    ADCSRA=0xCF;
}

ushort adc_read(uchar ch)
{
    uchar admux = ADMUX;

    // Select ADC input
    ADMUX = ((ch & 0x0F)|(ADC_VREF_TYPE & 0xC0));

    // Delay needed for the stabilization of the ADC input voltage
    if(admux != ADMUX) delay_us(10);

    // Start the AD conversion
    ADCSRA |= 0x40;
    
    // Wait for the AD conversion to complete
    while ((ADCSRA & 0x10) == 0);
    ADCSRA |= 0x10;

    return ADCW;
}


#endif /* __AVR__ */
