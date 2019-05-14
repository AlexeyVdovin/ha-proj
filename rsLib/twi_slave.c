#ifndef __AVR__

#else /* __AVR__ */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <util/delay.h>

#include "twi_slave.h"

/* -------------------------------------------------------------------------------------------------------------
Command:
S ADDR W A | CMD | ARGS | RS ADR R A | RET | DATA | P
---------------------------------------------------------------------------------------------------------------- */

volatile uint8_t regs[64];
volatile uint8_t twi_state;

void led_red_on();
void led_red_off();
void led_green_on();
void led_green_off();

ISR(TWI_vect)
{
    static ushort cnt = 0;
    static uint8_t adr = 0, n = 0;
    uint8_t data = 0x77;
    uint8_t cr = 0;
    twi_state = TWSR;

    switch(twi_state & 0xF8)
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
                else
                {
                    n = adr & 0x0F;
                    switch(n)
                    {
                        case 0:
                            break;
                        case 1:
                            PORTB = (PORTB & ~0x04) | (data ? 0x04 : 0);
                            regs[0x14] = (regs[0x14] & ~CTRL_RELAY_1) | (data ? CTRL_RELAY_1 : 0);
                            break;
                        case 2:
                            PORTB = (PORTB & ~0x02) | (data ? 0x02 : 0);
                            regs[0x14] = (regs[0x14] & ~CTRL_RELAY_2) | (data ? CTRL_RELAY_2 : 0);
                            break;
                        case 3:
                            PORTD = (PORTD & ~0x80) | (data ? 0x80 : 0);
                            regs[0x14] = (regs[0x14] & ~CTRL_RELAY_3) | (data ? CTRL_RELAY_3 : 0);
                            break;
                        case 4:
                            PORTB = (PORTB & ~0x01) | (data ? 0x01 : 0);
                            regs[0x14] = (regs[0x14] & ~CTRL_RELAY_4) | (data ? CTRL_RELAY_4 : 0);
                            break;
                        case 5:
                        case 6:
                        case 7:
                            break;
                        case 8:
                            if(data) led_red_on(); else led_red_off();
                            break;
                        case 9:
                            if(data) led_green_on(); else led_green_off();
                            break;
                        default:
                            break;
                    }
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
    TWCR = (TWCR & (~(1<<TWEA))) | (1<<TWINT) | cr;
}

void twi_init(uint8_t address)
{
    uint8_t i;

    TWAR = (address << 1);
    // Enable address matching, clear TWINT, enable TWI interrupt
    TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
    for(i = 0; i < sizeof(regs)/sizeof(regs[0]); ++i) regs[i] = 0;
}

void twi_enable()
{
    TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
}

uint8_t twi_disable()
{
    uint8_t i;
    TWCR = 0;
    DDRC = DDRC & (~0x30);
    _delay_us(10);

    // SCL is Low -> Fail
    if((PINC & 0x20) == 0) return 1;

    // SDA is Low -> Resume
    if((PINC & 0x10) == 0)
    {
        PORTC = PORTC & (~0x30);
        for(i = 0; i < 20; i ++)
        {
            // Toggle SCL line
            DDRC = DDRC | 0x20;    // SCL -> 0
            _delay_us(10);
            DDRC = DDRC & (~0x20); // SCL -> 1
            _delay_us(10);
            if((PINC & 0x10) == 1)
            {
                // Stop condition
                DDRC = DDRC | 0x20;    // SCL -> 0
                _delay_us(10);
                DDRC = DDRC | 0x10;    // SDA -> 0
                _delay_us(10);
                DDRC = DDRC & (~0x20); // SCL -> 1
                _delay_us(10);
                DDRC = DDRC & (~0x10); // SDA -> 1
                break;
            }
        }
        DDRC = DDRC & (~0x30);
        if(i == 20) return 2;
    }
    return 0;
}

void* get_reg(uint8_t adr)
{
    return (adr < sizeof(regs)/sizeof(regs[0]) ? regs+adr : 0);
}

#endif /* __AVR__ */
