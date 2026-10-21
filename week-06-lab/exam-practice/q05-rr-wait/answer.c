/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q5: your answer. See STATEMENT.md.
 */

#include <stdint.h>

/* The most tasks a question uses. */
#define RR_MAX_TASKS 8

/* (a) */
uint32_t rr_finish_time( const uint32_t *burst, unsigned n, unsigned q, unsigned i )
{
    ( void ) burst;
    ( void ) n;
    ( void ) q;
    ( void ) i;
    return 0;                   /* replace with your code */
}

/* (b) */
uint32_t rr_avg_wait_x100( const uint32_t *burst, unsigned n, unsigned q )
{
    ( void ) burst;
    ( void ) n;
    ( void ) q;
    return 0;                   /* replace with your code */
}

/* (c) */
unsigned rr_switches( const uint32_t *burst, unsigned n, unsigned q )
{
    ( void ) burst;
    ( void ) n;
    ( void ) q;
    return 0;                   /* replace with your code */
}
