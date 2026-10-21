/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q4: your answer. See STATEMENT.md.
 */

#include <stdint.h>

/* (a) */
void frame_hw( uint32_t *frame, void ( *entry )( void ), void ( *exit_fn )( void ) )
{
    ( void ) frame;
    ( void ) entry;
    ( void ) exit_fn;           /* replace with your code */
}

/* (b) */
uint32_t *frame_init( uint32_t *stack_top, void ( *entry )( void ), void ( *exit_fn )( void ) )
{
    ( void ) entry;
    ( void ) exit_fn;
    return stack_top;           /* replace with your code */
}

/* (c) */
void frame_set_pc( uint32_t *saved_sp, void ( *new_entry )( void ) )
{
    ( void ) saved_sp;
    ( void ) new_entry;         /* replace with your code */
}
