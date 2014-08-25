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

ISR(ADC_vect)
{

}

void adc_init()
{
    // ADC Clock frequency: 93.750 kHz
    // ADC Voltage Reference: Int., cap. on AREF
    ADMUX=ADC_IN1 | (ADC_VREF_TYPE & 0xff);
    ADCSRA=0xCF;
}

ushort adc_read(uchar ch)
{
    // Select ADC input
    ADMUX = (ch | (ADC_VREF_TYPE & 0xff));

    // Delay needed for the stabilization of the ADC input voltage
    delay_us(10);

    // Start the AD conversion
    ADCSRA |= 0x40;
    
    // Wait for the AD conversion to complete
    while ((ADCSRA & 0x10) == 0);
    ADCSRA |= 0x10;

    return ADCW;
}


#endif /* __AVR__ */
