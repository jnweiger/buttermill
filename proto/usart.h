/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART declarations
 *
 * $Id: uart.h,v 1.1 2005/12/28 21:38:59 joerg_wunsch Exp $
 */

//#ifndef F_CPU
//# define F_CPU 8000000L
//#endif
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>

/*
 * Perform UART startup initialization.
 */
void    usart_9600_1mhz(void);
void	usart_9600_8mhz(void);
void	usart_9600_7mhz(void);
void    uart_init(void);

/*
 * Send one character to the UART.
 */
int	uart_putchar(char c, FILE *stream);

/*
 * Speed of the serial line
 * e.g. 9600, 19200, 38400, 57600, 115200
 */
#define UART_BAUD 9600
#define U2X 1
/*
 * Receive one character from the UART.  The actual reception is
 * line-buffered, and one character is returned from the buffer at
 * each invokation.
 */
int	uart_getchar(FILE *stream);
