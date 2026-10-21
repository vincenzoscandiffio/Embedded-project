/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q3: a place to try your functions. It includes your answer.c, so
 * the functions and the types of the question are in scope. Add the
 * calls you want. It checks nothing.
 */

#include "console.h"
#include "answer.c"          /* your functions, from the question folder */

int main( void )
{
    kprintf( "mpu_size_field( 4096 ) = %u\n", mpu_size_field( 4096 ) );
    kprintf( "mpu_rasr( 4096, 0 ) = 0x%x\n", mpu_rasr( 4096, 0 ) );
    kprintf( "mpu_base_ok( 0x20000000, 4096 ) = %d\n", mpu_base_ok( 0x20000000, 4096 ) );
    return 0;
}
