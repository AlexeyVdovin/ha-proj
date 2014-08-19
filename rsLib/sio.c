#ifndef __AVR_XMEGA__

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "sio.h"

static int port = -1;
static const char* devname = "/dev/ttyUSB0";

#define AFTERX(x) B ## x
#define XTTY_BAUD(x) AFTERX(x)
#define TTY_BAUD XTTY_BAUD(BAUD)

void sio_init()
{
    struct termios tty;
    port = open(devname, O_RDWR | O_NDELAY | O_NOCTTY );
    if(port < 0)
    {
        printf("Invalid COM port. Error: %d\n", errno);
        exit(1);
    }
    
	if(tcgetattr(port, &tty) < 0)
    {
        printf("tcgetattr() failed. Error: %d\n", errno);
        exit(1);
    }

    // Reset terminal to RAW mode
    cfmakeraw(&tty);

    // Set baud rate
    cfsetspeed(&tty, TTY_BAUD);

    if(tcsetattr(port, TCSAFLUSH, &tty) < 0)
    {
        printf("tcsetattr() failed. Error: %d\n", errno);
        exit(1);
    }
}

char sio_putchar(char c)
{
    return write(port, &c, 1);
}

int	sio_getchar()
{
    uchar c;
    if(read(port, &c, 1) <= 0) return -1;
    return (c & 0x00FF);
}

uchar sio_rxcount()
{
    int bytes_avail = 0;
    ioctl(port, FIONREAD, &bytes_avail);
    if(bytes_avail > 255) bytes_avail = 255;
    return (uchar)bytes_avail;
}
#else
#include "defines.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define Tx_On()  (PORTD |=  0x04)
#define Tx_Off() (PORTD &= ~0x04)
#define Rx_On()  (PORTD &= ~0x08)
#define Rx_Off() (PORTD |=  0x08)

#define Led_On()    (PORTC |=  0x02)
#define Led_Off()   (PORTC &= ~0x02)

#define TXB8 0
#define RXB8 1
#define UPE  2
#define OVR  3
#define FE   4
#define UDRE 5
#define RXC  7

#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR  (1<<UPE)
#define DATA_OVERRUN  (1<<OVR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE   (1<<RXC)

/*
 * Size of internal buffer UART
 */
#define RX_BUFFER_SIZE 32
#define TX_BUFFER_SIZE 32

#define RX_BUFFER_MASK (RX_BUFFER_SIZE-1)
#define TX_BUFFER_MASK (TX_BUFFER_SIZE-1)

volatile char rx_buffer[RX_BUFFER_SIZE];
volatile char tx_buffer[TX_BUFFER_SIZE];

volatile uchar rx_wr_index,rx_rd_index,rx_counter;
volatile uchar tx_wr_index,tx_rd_index,tx_counter;

volatile union {
// This flag is set on USART Receiver buffer overflow
 char rx_buffer_overflow:1;
} uart_flags;

// USART Receiver interrupt service routine
ISR(USART_RXC_vect)
{
  char status, data;
  status = UCSRA;
  data = UDR;
  
  if((status & (FRAMING_ERROR|PARITY_ERROR|DATA_OVERRUN)) == 0)
  {
    rx_buffer[rx_wr_index++] = data;
    if(rx_wr_index == RX_BUFFER_SIZE) rx_wr_index = 0;
    if(++rx_counter == RX_BUFFER_SIZE) 
    {
      rx_counter = 0;
      uart_flags.rx_buffer_overflow = 1;
    }
  }
}

// USART Transmitter interrupt service routine
ISR(USART_TXC_vect)
{
    Led_Off();
    Tx_Off();
    Rx_On();
}

ISR(USART_UDRE_vect)
{
    if(tx_counter)
    {
        --tx_counter;
        UDR = tx_buffer[tx_rd_index++];
        if(tx_rd_index == TX_BUFFER_SIZE) tx_rd_index = 0;
    }
    else
    {
        UCSRB &= ~_BV(UDRIE);
    }
}

/*
 * Initialize the UART to 'baud' Bd, tx/rx, 8N1.
 *
 */
void
uart_init(void)
{
  uart_flags.rx_buffer_overflow = 0;
  rx_wr_index = rx_rd_index = rx_counter = 0;
  tx_wr_index = tx_rd_index = tx_counter = 0;
  UCSRC = 0x06;
  UBRRH = 0x00;

#include <util/setbaud.h>
   UBRRH = UBRRH_VALUE;
   UBRRL = UBRRL_VALUE;
#if USE_2X
   UCSRA |= _BV(U2X);
#else
   UCSRA &= ~_BV(U2X);
#endif

  UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE) | _BV(TXCIE) | _BV(UDRIE); /* tx/rx enable */
  
  Led_Off();
  Tx_Off();
  Rx_On();
}

/*
 * Send character c down the UART Tx, or put to Tx queue
 *
 */
char sio_putchar(char c)
{
  while(tx_counter == TX_BUFFER_SIZE);

  cli();
  tx_buffer[tx_wr_index++] = c;
  ++tx_counter;
  if(tx_wr_index == TX_BUFFER_SIZE) tx_wr_index = 0;
  Rx_Off();
  Tx_On();
  Led_On();
  UCSRB |= _BV(UDRIE);
  sei();

  return 0;
}

/*
 * Receive a character from the UART Rx.
 *
 */
int sio_getchar(void)
{
  char data;

  if(rx_counter == 0)
    return -1; /* EOF */

  data = rx_buffer[rx_rd_index++];

  if (rx_rd_index == RX_BUFFER_SIZE)
    rx_rd_index = 0;

  cli();
  --rx_counter;
  sei();

  return (data & 0x00FF);
}

uchar sio_rxcount(void)
{
    return rx_counter;
}

#endif
