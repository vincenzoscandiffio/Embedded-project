/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * The console of the practice questions: UART0, polled, no interrupts.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

/* Switch on the transmitter of UART0. Called by Reset_Handler before
 * main(): a question's program does not call it. */
void console_init( void );

/* Format and print, as kprintf of week 3: %c %s %d %u %x %% (no width,
 * no precision). */
void kprintf( const char *fmt, ... );

#endif /* CONSOLE_H */
