/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q2: a place to try your functions. It includes your answer.c, so
 * the functions and the types of the question are in scope. Add the
 * calls you want. It checks nothing.
 */

#include "console.h"
#include "answer.c"          /* your functions, from the question folder */

int main( void )
{
    struct ring r = { { 0 }, 0, 0 };        /* an empty ring */
    uint8_t byte = 0;

    kprintf( "put 10: %d\n", ring_put( &r, 10 ) );
    kprintf( "put 20: %d\n", ring_put( &r, 20 ) );
    kprintf( "count: %u\n", ring_count( &r ) );
    kprintf( "get: %d, byte %u\n", ring_get( &r, &byte ), byte );
    return 0;
}
