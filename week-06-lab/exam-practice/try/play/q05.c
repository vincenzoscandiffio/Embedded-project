/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q5: a place to try your functions. It includes your answer.c, so
 * the functions and the types of the question are in scope. Add the
 * calls you want. It checks nothing.
 */

#include "console.h"
#include "answer.c"          /* your functions, from the question folder */

int main( void )
{
    uint32_t burst[] = { 4, 2, 6 };         /* three tasks, ready at tick 0 */

    for ( unsigned i = 0; i < 3; i++ )
    {
        kprintf( "task %u finishes at tick %u\n", i, rr_finish_time( burst, 3, 2, i ) );
    }
    kprintf( "average wait x100: %u\n", rr_avg_wait_x100( burst, 3, 2 ) );
    kprintf( "switches: %u\n", rr_switches( burst, 3, 2 ) );
    return 0;
}
