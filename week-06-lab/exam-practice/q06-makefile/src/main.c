/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q6: the main of the program the Makefile builds.
 */

#include "board.h"
#include "util.h"

void led_set( unsigned value );

int main( void )
{
    led_set( ( unsigned ) add( 1, 2 ) % LED_COUNT );
    return 0;
}
