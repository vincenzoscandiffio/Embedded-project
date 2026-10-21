/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q5: the reference solution.
 */

#include <stdint.h>

/* The most tasks a question uses. */
#define RR_MAX_TASKS 8

/* Run the whole schedule once. All tasks are ready at tick 0, so the
 * order of the queue is the order of the array and never changes: a task
 * that is not finished goes to the end, which for a queue that is only
 * walked round and round is the same position next time round. Fills
 * finish[] and returns the number of context switches. */
static unsigned simulate( const uint32_t *burst, unsigned n, unsigned q, uint32_t *finish )
{
    uint32_t left[RR_MAX_TASKS];        /* ticks each task still needs */
    unsigned unfinished = n;
    unsigned last = n;                  /* the task that ran last: none yet */
    unsigned switches = 0;
    uint32_t now = 0;

    for ( unsigned i = 0; i < n; i++ )
    {
        left[i] = burst[i];
    }
    while ( unfinished > 0 )
    {
        for ( unsigned i = 0; i < n; i++ )
        {
            if ( left[i] == 0 )
            {
                continue;               /* finished: not in the queue */
            }
            if ( last != n && last != i )
            {
                switches++;             /* the processor changes task */
            }
            last = i;
            uint32_t run = left[i] < q ? left[i] : q;   /* a short burst ends the slice early */
            now += run;
            left[i] -= run;
            if ( left[i] == 0 )
            {
                finish[i] = now;
                unfinished--;
            }
        }
    }
    return switches;
}

/* (a) */
uint32_t rr_finish_time( const uint32_t *burst, unsigned n, unsigned q, unsigned i )
{
    uint32_t finish[RR_MAX_TASKS];
    simulate( burst, n, q, finish );
    return finish[i];
}

/* (b) Waiting time = finish - burst (arrival 0). Sum, then one division,
 * so the average is truncated once. */
uint32_t rr_avg_wait_x100( const uint32_t *burst, unsigned n, unsigned q )
{
    uint32_t sum = 0;
    for ( unsigned i = 0; i < n; i++ )
    {
        sum += rr_finish_time( burst, n, q, i ) - burst[i];
    }
    return sum * 100 / n;
}

/* (c) */
unsigned rr_switches( const uint32_t *burst, unsigned n, unsigned q )
{
    uint32_t finish[RR_MAX_TASKS];
    return simulate( burst, n, q, finish );
}
