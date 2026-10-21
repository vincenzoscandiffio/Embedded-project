/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * The console of the practice questions: see console.h. The same UART0
 * registers as drivers/uart.c of the kernel, used by polling: wait
 * until the send buffer has room, then write the byte.
 */

#include <stdarg.h>
#include <stdint.h>
#include "console.h"
#include "format.h"

/* UART0 of the board (CMSDK), the registers that matter here. */
#define UART0_DATA      ( *( volatile uint32_t * ) 0x40004000UL )
#define UART0_STATE     ( *( volatile uint32_t * ) 0x40004004UL )
#define UART0_CTRL      ( *( volatile uint32_t * ) 0x40004008UL )
#define UART0_BAUDDIV   ( *( volatile uint32_t * ) 0x40004010UL )
#define UART_STATE_TXBF ( 1UL << 0 )    /* send buffer full */
#define UART_CTRL_TXEN  ( 1UL << 0 )    /* transmitter enabled */

void console_init( void )
{
    UART0_BAUDDIV = 16;                 /* QEMU ignores the speed */
    UART0_CTRL = UART_CTRL_TXEN;
}

/* The output function for vformat: send one character. */
static void console_out( char c, void *ctx )
{
    ( void ) ctx;
    while ( UART0_STATE & UART_STATE_TXBF )
    {
    }
    UART0_DATA = ( uint32_t ) ( unsigned char ) c;
}

void kprintf( const char *fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );
    vformat( console_out, 0, fmt, ap );
    va_end( ap );
}
