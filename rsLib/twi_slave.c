#ifndef __AVR__

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "twi_slave.h"

/* -------------------------------------------------------------------------------------------------------------
Command:
S ADDR W A | CMD | ARGS | RS ADR R A | RET | DATA | P
---------------------------------------------------------------------------------------------------------------- */

uint8_t regs[64];

ISR(TWI_vect)
{
    static ushort cnt = 0;
    static uint8_t adr = 0, n = 0;
    uint8_t data = 0x77;
    uint8_t cr = 0;

    switch(TWSR & 0xF8)
    {
        case TW_SR_SLA_ACK:
        { // SLA+W received, ACK returned
            *(ushort*)get_reg(8*2) = ++cnt; // Update activity reg
            n = 0;
            cr = (1<<TWEA);

            break;
        }
        case TW_ST_SLA_ACK:
        { // SLA+R received, ACK returned
            if(n == 1)
            {
                if(adr < sizeof(regs)/sizeof(regs[0]))
                {
                    data = regs[adr];
                    cr = (1<<TWEA);
                }
                else
                {
                    data = 0xFF;
                }
                TWDR = data;
                ++adr;
            }
            break;
        }
        case TW_SR_DATA_ACK:
        { // data received, ACK returned
            data = TWDR;
            if(n == 0) ++n, adr = data;
            else
            {
                if(adr < sizeof(regs)/sizeof(regs[0]))
                {
                    regs[adr] = data;
                    cr = (1<<TWEA);
                }
                // Prepare for next register reception
                n = 1;
                ++adr;
            }
            break;
        }
        case TW_ST_DATA_ACK:
        { // data transmitted, ACK received
            if(n == 1)
            {
                if(adr < sizeof(regs)/sizeof(regs[0]))
                {
                    data = regs[adr];
                    cr = (1<<TWEA);
                }
                else
                {
                    data = 0xFF;
                }
                TWDR = data;
                ++adr;
            }
            break;
        }
        case TW_ST_LAST_DATA:
        { // last data byte transmitted, ACK received

            break;
        }
        case TW_SR_STOP:
        { // Stop or Repeated start

            break;
        }
        default:
        {
            break;
        }

    }
    // clear TWI interrupt flag, prepare to receive next byte
    TWCR |= (1<<TWIE) | (1<<TWINT) | cr | (1<<TWEN);
}

void twi_init(uint8_t address)
{
    uint8_t i;

    TWAR = (address << 1);
    // Enable address matching, clear TWINT, enable TWI interrupt
    TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
    for(i = 0; i < sizeof(regs)/sizeof(regs[0]); ++i) regs[i] = 0;
}

void* get_reg(uint8_t adr)
{
    return (adr < sizeof(regs)/sizeof(regs[0]) ? regs+adr : 0);
}

#endif /* __AVR__ */
