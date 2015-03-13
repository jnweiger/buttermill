/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART implementation
 *
 * $Id: uart.c,v 1.1 2005/12/28 21:38:59 joerg_wunsch Exp $
 */
#include "cpu_mhz.h"
#include "usart.h"

/*
 * Initialize the UART to 9600 Bd, tx/rx, 8N1.
 */
void
uart_init(void)
{
#if F_CPU < 2000000UL && defined(U2X)
  UCSRA = _BV(U2X);             /* improve baud rate error by using 2x clk */
  UBRRL = (F_CPU / (8UL * UART_BAUD)) - 1;
#else
  UBRRL = (F_CPU / (16UL * UART_BAUD)) - 1;
#endif
  UCSRB = _BV(TXEN) | _BV(RXEN); /* tx/rx enable */
}

void
usart_9600_7mhz(void)
{
#  undef F_CPU
#  define F_CPU 7372800L
#  define BAUD 9600
#  include <util/setbaud.h>
   UBRRH = UBRRH_VALUE;
   UBRRL = UBRRL_VALUE;
   #if USE_2X
   UCSRA |= (1 << U2X);
   #else
   UCSRA &= ~(1 << U2X);
   #endif
  UCSRB = _BV(TXEN) | _BV(RXEN); /* tx/rx enable */
}

void
usart_9600_1mhz(void)
{
#  undef F_CPU
#  define F_CPU 1843200L
#  define BAUD 9600
#  include <util/setbaud.h>
   UBRRH = UBRRH_VALUE;
   UBRRL = UBRRL_VALUE;
   #if USE_2X
   UCSRA |= (1 << U2X);
   #else
   UCSRA &= ~(1 << U2X);
   #endif
  UCSRB = _BV(TXEN) | _BV(RXEN); /* tx/rx enable */
}

void
usart_9600_8mhz(void)
{
#  undef F_CPU
#  define F_CPU 8000000L
#  define BAUD 9600
#  include <util/setbaud.h>
   UBRRH = UBRRH_VALUE;
   UBRRL = UBRRL_VALUE;
   #if USE_2X
   UCSRA |= (1 << U2X);
   #else
   UCSRA &= ~(1 << U2X);
   #endif
  UCSRB = _BV(TXEN) | _BV(RXEN); /* tx/rx enable */
}

/*
 * Send character c down the UART Tx, wait until tx holding register
 * is empty.
 */
int
uart_putchar(char c, FILE *stream)
{
  if (c == '\n')
    uart_putchar('\r', stream);
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = c;

  return 0;
}

/*
 * Receive a character from the UART Rx.
 */
int
uart_getchar(FILE *stream)
{
  uint8_t c;

  loop_until_bit_is_set(UCSRA, RXC);
  if (UCSRA & _BV(FE))
    return _FDEV_EOF;
  if (UCSRA & _BV(DOR))
    return _FDEV_ERR;
  c = UDR;
  /* behaviour similar to Unix stty ICRNL */
  if (c == '\r')
    c = '\n';

  return c;
}


#if 0
#ifndef UART_BAUD
# include "uart.h"
#endif
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
int main()
{
  uart_init();
  stdin = stdout = &uart_str;
//  for (;;)
//    {
//      printf("Enter a key: ");
      //int a = getchar();
//      printf("\n ascii_code=%d char='%c' seen\n\n", a, a);
//    }
      printf("Hallo world\n");
}
#endif
