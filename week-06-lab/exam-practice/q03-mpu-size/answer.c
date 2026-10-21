/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q3: your answer. See STATEMENT.md.
 */

#include <stdint.h>

/* (a) */
uint32_t mpu_size_field( uint32_t bytes )
{
    ( void ) bytes;
    return 0;                   /* replace with your code */
}

/* (b) */
uint32_t mpu_rasr( uint32_t bytes, int read_only )
{
    ( void ) bytes;
    ( void ) read_only;
    return 0;                   /* replace with your code */
}

/* (c) */
int mpu_base_ok( uint32_t base, uint32_t bytes )
{
    ( void ) base;
    ( void ) bytes;
    return 0;                   /* replace with your code */
}
