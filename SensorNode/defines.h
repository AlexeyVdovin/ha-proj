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
// #define BAUD                      57600
#define BAUD                      9600

/* Timer */
#define TIMER_ISR   TIMER2_COMP_vect
#define TIMER_TCCR  TCCR2
#define TIMER_OCR   OCR2
#define TIMER_TCNT  TCNT2
#define TIMER_ASSR  ASSR

/* SIO */
#define Tx_On()  (PORTD |=  0x04)
#define Tx_Off() (PORTD &= ~0x04)
#define Rx_On()  (PORTD &= ~0x08)
#define Rx_Off() (PORTD |=  0x08)

#define Led_On()    (PORTC |=  0x02)
#define Led_Off()   (PORTC &= ~0x02)

#define RX_BUFFER_SIZE 32
#define TX_BUFFER_SIZE 32

#define SIO_RXC_ISR  USART_RX_vect
#define SIO_TXC_ISR  USART_TX_vect
#define SIO_UDRE_ISR USART_UDRE_vect

#define SIO_UCSRA   UCSR0A
#define SIO_UDR     UDR0
#define SIO_UCSRB   UCSR0B
#define SIO_UDRIE   UDRIE0
#define SIO_UCSRC   UCSR0C
#define SIO_UBRRH   UBRR0H
#define SIO_UBRRL   UBRR0L
#define SIO_U2X     U2X0
#define SIO_TXEN    TXEN0
#define SIO_RXEN    RXEN0
#define SIO_TXCIE   TXCIE0
#define SIO_RXCIE   RXCIE0

/* ADC */

/* PWM */
#define PWM_CH1     OCR1A
#define PWM_CH2     OCR1B
/* Clock Select Description
    0 - Off
    1 - 1/1
    2 - 1/8
    3 - 1/64
    4 - 1/256
    5 - 1/1024
*/
#define PWM_FREQ(n) (TCCR1B = (TCCR1B & 0xF8) | ((n) & 0x07))

/* 1-WIRE */
#define DS1W_PORT           C // Port C
#define DS1W_PIN            0 // Bit #0
#define DS1820_MAXPROBES    4

#endif
