/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q4: a place to try your functions. It includes your answer.c, so
 * the functions and the types of the question are in scope. Add the
 * calls you want. It checks nothing.
 */

#include "console.h"
#include "answer.c"          /* your functions, from the question folder */

static void my_task( void )
{
}

static void my_exit( void )
{
}

/* An array to use as a stack. Its top is the first address above it. */
static uint32_t stack[ 32 ];

int main( void )
{
    uint32_t *sp = frame_init( &stack[ 32 ], my_task, my_exit );

    kprintf( "saved sp is %u words below the top\n", ( unsigned ) ( &stack[ 32 ] - sp ) );
    for ( int i = 0; i < 16; i++ )
    {
        kprintf( "word %d: 0x%x\n", i, sp[ i ] );
    }
    return 0;
}
