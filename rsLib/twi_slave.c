#ifndef __AVR__

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "twi_slave.h"

ISR(TWI_vect)
{
    uint8_t data;
    
    switch(TWSR & 0xF8)
    {
        case TW_SR_SLA_ACK:
        { // device has been addressed
            // clear TWI interrupt flag, prepare to receive next byte and acknowledge
		    TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN); 
            break;
        }
        case TW_SR_DATA_ACK:
        { // data has been received in slave receiver mode
            data = TWDR;

            break;
        }
        case TW_ST_DATA_ACK:
        { // device has been addressed to be a transmitter
            TWDR = data;
         
            break;
        }
        default:
        {
            // if none of the above apply prepare TWI to be addressed again
		    TWCR |= (1<<TWIE) | (1<<TWEA) | (1<<TWEN);
        }
            
    }            
}

void twi_init(uint8_t address)
{
    TWAR = (address << 1);
	// set the TWCR to enable address matching and enable TWI, clear TWINT, enable TWI interrupt
	TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
}

#endif /* __AVR__ */
