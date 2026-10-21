/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q1: a place to try your functions. It includes your answer.c, so
 * the functions and the types of the question are in scope. Add the
 * calls you want. It checks nothing.
 */

#include "console.h"
#include "answer.c"          /* your functions, from the question folder */

int main( void )
{
    kprintf( "tick_before( 3, 7 ) = %d\n", tick_before( 3, 7 ) );
    kprintf( "tick_wait_left( 100, 130 ) = %u\n", tick_wait_left( 100, 130 ) );
    return 0;
}
