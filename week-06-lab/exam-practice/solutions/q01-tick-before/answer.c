/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q1: the reference solution.
 */

#include <stdint.h>

/* (a) The unsigned difference a - b is taken modulo 2^32; read as a
 * signed number it is negative exactly when a is earlier than b, as
 * long as the two ticks are less than 2^31 apart. Comparing a < b
 * would be wrong across the wrap: 0xFFFFFFFA is earlier than 2. */
int tick_before( uint32_t a, uint32_t b )
{
    return ( int32_t ) ( a - b ) < 0;
}

/* (b) Still to wait: the difference, if wake is in the future; else 0. */
uint32_t tick_wait_left( uint32_t now, uint32_t wake )
{
    return tick_before( now, wake ) ? wake - now : 0;
}
