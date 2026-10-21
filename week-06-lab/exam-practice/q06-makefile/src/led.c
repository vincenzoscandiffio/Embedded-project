/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q6: a source of the program the Makefile builds.
 */

#include "board.h"

void led_set( unsigned value )
{
    LED_REG = value;
}
