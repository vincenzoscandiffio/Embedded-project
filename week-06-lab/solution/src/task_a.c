/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Task A. It runs unprivileged, so it only uses system calls
 * (lib/syscalls.h) and the helpers built on them (lib/printf.h).
 *
 * Normal build: A does A_STEPS steps of busy work (WORK_TICKS ticks
 * each), prints a line after each one, and returns. It never gives the
 * CPU away itself: only the scheduler's time slice (QUANTUM ticks)
 * lets task B run. Each line shows the tick, so the output shows the
 * turns: about QUANTUM / WORK_TICKS lines of A, then of B, and so on.
 *
 * Demo builds (see the Makefile):
 *     make DEMO=coop run  - A loops for ever and never yields, and the
 *                           kernel does not preempt (QUANTUM = 0):
 *                           B never runs (it starves).
 *     make DEMO=idle run  - A works one tick, then sleeps 10 ticks with
 *                           task_delay, so the idle task gets the CPU.
 */

#include <stdint.h>
#include "src/tasks.h"
#include "lib/syscalls.h"
#include "lib/printf.h"

/* How many steps task A does before it returns. */
#define A_STEPS 10
/* Ticks of busy work in one step (2 ticks = 20 ms). */
#define WORK_TICKS 2

/* The number of the current step. A global on purpose: the linker
 * script puts every variable of this file inside task A's partition. */
#if !defined( DEMO_RESTART_ALONE )
static uint32_t a_step = 0;
#endif

void task_a( void )
{
    printf( "A: hello from task A\n" );

#if defined( DEMO_COOP )
    /* Cooperative scheduling: the kernel never takes the CPU away, so
     * a task that never gives it back keeps it for ever. Add
     * task_yield(); at the end of the loop body (and in task B) and the
     * two tasks alternate again, one step each. */
    for ( ;; )
    {
        a_step++;
        printf( "A: step %u (tick %u)\n", a_step, task_tick_count() );
        spin_ticks( WORK_TICKS );           /* work, never yield */
    }
#elif defined( DEMO_RESTART_ALONE )
    /* Nothing to do: return at once. A finishes, B is alone. */
    return;
#elif defined( DEMO_IDLE )
    /* Short work, long sleep: most of the time A is BLOCKED. task_delay
     * counts from the moment it is called, so the step period is the
     * work plus the delay (1 + 10 = 11 ticks, or a little more). */
    for ( a_step = 1; a_step <= 5; a_step++ )
    {
        printf( "A: step %u (tick %u)\n", a_step, task_tick_count() );
        spin_ticks( 1 );                    /* one tick of work */
        task_delay( 10 );                   /* BLOCKED for 10 ticks */
    }
#else
    /* Busy work only: A gives the CPU away only when the time slice
     * ends (preemption) or when it returns. */
    for ( a_step = 1; a_step <= A_STEPS; a_step++ )
    {
        spin_ticks( WORK_TICKS );           /* one step of work */
        printf( "A: step %u (tick %u)\n", a_step, task_tick_count() );
    }
#endif
#if defined( DEMO_RESTART ) || defined( DEMO_RESTART_DEEP )
    /* The demo ends A's run with the trace, restarts included. */
    trace_dump();
#endif
    /* Returning ends the task: the lr of its first frame is task_exit. */
}
