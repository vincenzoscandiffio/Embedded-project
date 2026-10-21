/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q6: the main of a second program, a check of `add`.
 */

#include "board.h"
#include "util.h"

/* Returns 0 when add() gives the right sum. */
int main( void )
{
    return add( LED_COUNT, 1 ) != 9;
}
