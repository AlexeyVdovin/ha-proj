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
#define BAUD                      57600

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

#define SIO_RXC_ISR  USART_RXC_vect
#define SIO_TXC_ISR  USART_TXC_vect
#define SIO_UDRE_ISR USART_UDRE_vect

#define SIO_UCSRA   UCSRA
#define SIO_UDR     UDR
#define SIO_UCSRB   UCSRB
#define SIO_UDRIE   UDRIE
#define SIO_UCSRC   UCSRC
#define SIO_UBRRH   UBRRH
#define SIO_UBRRL   UBRRL
#define SIO_U2X     U2X
#define SIO_TXEN    TXEN
#define SIO_RXEN    RXEN
#define SIO_TXCIE   TXCIE
#define SIO_RXCIE   RXCIE
#define SIO_UDRIE   UDRIE

/* ADC */


#endif
