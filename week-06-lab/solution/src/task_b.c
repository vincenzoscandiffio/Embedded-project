/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Task B. It runs unprivileged, like task A.
 *
 * Normal build: the same busy work as A, with more steps (B_STEPS), so
 * B goes on alone for a while after A has finished. Then it returns,
 * only the idle task is left, and the kernel says so. Just before it
 * returns, B asks the kernel to print the scheduler trace.
 *
 * Demo builds (see the Makefile):
 *     make DEMO=idle run  - B is periodic: it wakes every 6 ticks with
 *                           task_delay_until, exactly on time.
 *     make DEMO=rogue | badptr | overflow run
 *                         - B misbehaves on purpose, to show what each
 *                           protection layer does. B starts only after
 *                           A's first time slice, so the fault happens
 *                           after a context switch: the MPU region was
 *                           reprogrammed for B by the switch.
 */

#include <stdint.h>
#include "src/tasks.h"
#include "lib/syscalls.h"
#include "lib/printf.h"

/* How many steps task B does before it returns. */
#define B_STEPS 15
/* Ticks of busy work in one step (2 ticks = 20 ms), as in task A. */
#define WORK_TICKS 2

/* The number of the current step. A global on purpose: the linker
 * script puts every variable of this file inside task B's partition. */
static uint32_t b_step = 0;

#if defined( DEMO_ROGUE ) || defined( DEMO_BADPTR )
/* The kernel's own uptime counter (drivers/systick.c). Task B should
 * never touch it: it belongs to the kernel. */
extern volatile uint32_t uptime_ms;
#endif

#if defined( DEMO_OVERFLOW )
/* Recurse 4 levels deep (depth 0 to 3). Each call uses about 1 KB of
 * stack for its local array, so about 4 KB in all: 3 KB more than the
 * 1 KB stack has. The stack runs past the bottom of task B's partition
 * into the RAM below it. 4 levels are enough to show this, and few
 * enough to stay inside RAM. (The limit also lets the compiler see that
 * the recursion ends; without one, it warns about endless recursion.) */
#define EAT_DEPTH 3
static uint32_t eat_stack( uint32_t depth )
{
    volatile uint8_t pad[ 1024 ];           /* a big local array */
    pad[ 0 ] = (uint8_t) depth;             /* touch it so it is not optimised away */
    if ( depth >= EAT_DEPTH )
    {
        return pad[ 0 ];                    /* deep enough: stop */
    }
    return eat_stack( depth + 1 ) + pad[ 0 ];   /* go one level deeper */
}
#endif

#if defined( DEMO_RESTART ) || defined( DEMO_RESTART_ALONE ) || defined( DEMO_RESTART_DEEP )
/* A global with an initial value: it lives in B's .data. A task that
 * starts again gets its variables back as at the first start, so B
 * prints 100 at every start, not 101, 102... */
static uint32_t b_runs = 100;

#if defined( DEMO_RESTART_DEEP )
/* B ends with an explicit exit, made three calls deep: each function
 * has a local array, so the stack is well below its top when the
 * system call is made. */
static void leave_c( void )
{
    volatile uint32_t pad[ 8 ];
    pad[ 0 ] = 3;
    if ( pad[ 0 ] == 3 )
    {
        sys_exit();                         /* the exit system call */
    }
}
static void leave_b( void )
{
    volatile uint32_t pad[ 8 ];
    pad[ 0 ] = 2;
    if ( pad[ 0 ] == 2 )
    {
        leave_c();
    }
}
static void leave_a( void )
{
    volatile uint32_t pad[ 8 ];
    pad[ 0 ] = 1;
    if ( pad[ 0 ] == 1 )
    {
        leave_b();
    }
}
#endif

void task_b( void )
{
    printf( "B: start, b_runs = %u\n", b_runs );
    b_runs++;                               /* lost at the next restart */
    for ( b_step = 1; b_step <= 3; b_step++ )
    {
        spin_ticks( WORK_TICKS );           /* one step of work */
        printf( "B: step %u (tick %u)\n", b_step, task_tick_count() );
    }
#if defined( DEMO_RESTART_DEEP )
    leave_a();                              /* exit from a nested call */
#endif
    /* Returning is an exit too: B's policy turns either into a restart. */
}
#else
void task_b( void )
{
    printf( "B: hello from task B\n" );

#if defined( DEMO_ROGUE )
    /* Rogue write: overwrite the kernel's uptime counter. */
    printf( "B: writing to the kernel's uptime counter\n" );
    uptime_ms = 0x12345678;                 /* a store into kernel memory */
#endif

#if defined( DEMO_BADPTR )
    /* Bad pointer: ask the kernel to print 16 bytes that are in kernel
     * memory. The kernel runs privileged, so it could do it. */
    int r = sys_write( (const char *) &uptime_ms, 16 );
    printf( "\nB: sys_write returned %d\n", r );
#endif

#if defined( DEMO_OVERFLOW )
    /* Stack overflow: recurse until the stack leaves the partition. */
    printf( "B: recursing\n" );
    printf( "B: result %u\n", eat_stack( 0 ) );
#endif

#if defined( DEMO_IDLE )
    /* Periodic task: wake at last + 6, last + 12, ... exactly. The
     * time spent working does not shift the next wake-up, unlike
     * task_delay in task A. `last` is on B's stack, inside B's
     * partition, so the kernel accepts its address. */
    uint32_t last = task_tick_count();      /* the starting tick */
    for ( b_step = 1; b_step <= 5; b_step++ )
    {
        task_delay_until( &last, 6 );       /* BLOCKED until tick last + 6 */
        printf( "B: step %u (tick %u)\n", b_step, task_tick_count() );
        spin_ticks( 1 );                    /* one tick of work */
    }
#else
    /* Busy work only, as in task A. */
    for ( b_step = 1; b_step <= B_STEPS; b_step++ )
    {
        spin_ticks( WORK_TICKS );           /* one step of work */
        printf( "B: step %u (tick %u)\n", b_step, task_tick_count() );
    }
#endif

    /* Last action: print the trace, so the run ends with the chart. */
    trace_dump();

    /* Returning ends the task: the lr of its first frame is task_exit. */
}
#endif /* DEMO_RESTART */
