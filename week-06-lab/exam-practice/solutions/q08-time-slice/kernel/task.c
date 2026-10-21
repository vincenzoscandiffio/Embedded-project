/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Tasks and the round-robin scheduler, kernel side: see kernel/task.h.
 *
 * Q8: the reference solution: each task has its own time slice.
 * The rest of this file is the one of week-05/solution/.
 *
 * All of this runs inside an exception, privileged, on the main stack:
 *   - task_start_first(), task_block_until(), task_finish_current():
 *     inside SVCall (priority 0x80), on behalf of a system call;
 *   - task_increment_tick(): inside SysTick (priority 0xE0);
 *   - task_switch_context(): inside PendSV (priority 0xE0).
 * These never interrupt each other in the middle of their work:
 * SysTick and PendSV have the same priority, so neither can interrupt
 * the other, and SVCall only happens when a task runs `svc`, never
 * while a handler runs. So the task table needs no other protection.
 */

#include <stdint.h>
#include "cmsdk.h"
#include "kernel/task.h"
#include "kernel/mpu.h"
#include "kernel/kprintf.h"
#include "kernel/trace.h"
#include "src/tasks.h"
#include "lib/syscalls.h"

/* Addresses defined by the linker script. Declared as arrays: only
 * their address matters, there is no variable behind the name. */
extern uint32_t _task_a_part_start[], _task_a_stack_top[], _task_a_part_end[];
extern uint32_t _task_a_data_start[], _task_a_data_end[];
extern const uint32_t _task_a_data_load[];
extern uint32_t _task_b_part_start[], _task_b_stack_top[], _task_b_part_end[];
extern uint32_t _task_b_data_start[], _task_b_data_end[];
extern const uint32_t _task_b_data_load[];
extern uint32_t _idle_part_start[], _idle_stack_top[], _idle_part_end[];
extern uint32_t _idle_data_start[], _idle_data_end[];
extern const uint32_t _idle_data_load[];

/* The task table: one TCB per task. The application tasks come first,
 * in round-robin order; the idle task is always the last entry. The
 * table is not const any more: sp, state and wake_tick change. It is
 * not static: the trace (kernel/trace.c) reads the task names. */
struct tcb tasks[NUM_TASKS] = {
    {
        .name       = "A",
        .entry      = task_a,
        .stack_top  = _task_a_stack_top,
        .part_start = ( uint32_t ) _task_a_part_start,
        .part_end   = ( uint32_t ) _task_a_part_end,
        .data_start = _task_a_data_start,
        .data_end   = _task_a_data_end,
        .data_load  = _task_a_data_load,
        .slice      = A_SLICE,
    },
    {
        .name       = "B",
        .entry      = task_b,
        .stack_top  = _task_b_stack_top,
        .part_start = ( uint32_t ) _task_b_part_start,
        .part_end   = ( uint32_t ) _task_b_part_end,
        .data_start = _task_b_data_start,
        .data_end   = _task_b_data_end,
        .data_load  = _task_b_data_load,
        .slice      = B_SLICE,
    },
    {
        .name       = "idle",
        .entry      = idle_task,
        .stack_top  = _idle_stack_top,
        .part_start = ( uint32_t ) _idle_part_start,
        .part_end   = ( uint32_t ) _idle_part_end,
        .data_start = _idle_data_start,
        .data_end   = _idle_data_end,
        .data_load  = _idle_data_load,
        .slice      = 0,
    },
};

/* The application tasks are entries 0 .. NUM_APP_TASKS - 1 (A and B). */
#define NUM_APP_TASKS ( NUM_TASKS - 1 )
/* The idle task: the last entry. */
#define IDLE_TCB ( &tasks[NUM_TASKS - 1] )

/* See kernel/task.h. */
struct tcb *tcb_current = 0;
uint32_t tick_count = 0;

/* Ticks left in the time slice of the running task. */
static uint32_t quantum_left = QUANTUM;

/* Index of the application task that ran last: the round-robin scan
 * starts just after it. The idle task never changes it, so after an
 * idle period the turn goes on where it stopped. */
static unsigned rr_last = 0;

/* EXC_RETURN value (week 3): return to thread mode, on the process
 * stack (a task). */
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFDu

/* xPSR with only bit 24 (T, Thumb state) set. The CPU can only run
 * Thumb code: popping an xPSR without this bit would fault. */
#define XPSR_THUMB 0x01000000u

/* Copy the initial values of t's variables from FLASH into its
 * partition. Done once, before the task runs for the first time. */
static void partition_init( const struct tcb *t )
{
    const uint32_t *src = t->data_load;     /* initial values, in FLASH */
    uint32_t *dst = t->data_start;          /* the variables, in RAM */

    /* Copy word by word. The linker script stores .data and .bss
     * together in FLASH (the .bss part as zeros), so this one loop
     * sets every variable of the task, zeros included. */
    while ( dst < t->data_end )
    {
        *dst++ = *src++;                    /* copy one word, move both on */
    }
}

/* Build the first stack of task t, as if the task had already run and
 * been switched out by PendSV just before its first instruction. Then
 * the first switch to the task works exactly like every later one.
 * Seen from the top of the stack (high addresses) down:
 *
 *     stack_top ->  (first address above the stack)
 *                   xPSR = 0x01000000  \
 *                   pc   = entry        |
 *                   lr   = task_exit    |  8 words the hardware pops
 *                   r12  = 0            |  on exception return
 *                   r3, r2, r1, r0 = 0 /
 *                   r11 .. r4 = 0      -- 8 words PendSV pops itself
 *     t->sp     ->  (lowest used word: the saved r4)
 */
static void task_init_stack( struct tcb *t )
{
    /* The hardware frame: 8 words just below the top of the stack. */
    uint32_t *frame = t->stack_top - 8;
    frame[0] = 0;                           /* r0 */
    frame[1] = 0;                           /* r1 */
    frame[2] = 0;                           /* r2 */
    frame[3] = 0;                           /* r3 */
    frame[4] = 0;                           /* r12 */
    /* lr: where the task goes when its function returns. task_exit
     * (lib/syscalls.s) calls sys_exit, so returning means exit. */
    frame[5] = ( uint32_t ) task_exit;
    /* pc: where the task starts. A C function address has bit 0 set
     * (it means "Thumb code"); the saved pc must be a real instruction
     * address, so bit 0 is cleared. */
    frame[6] = ( uint32_t ) t->entry & ~1u;
    frame[7] = XPSR_THUMB;                  /* xPSR: only the Thumb bit */

    /* Below the frame: 8 more words for r4..r11, which PendSV saves
     * and restores by software. Their first values do not matter. */
    uint32_t *regs = frame - 8;
    for ( int i = 0; i < 8; i++ )
    {
        regs[i] = 0;                        /* r4 + i = 0 */
    }

    t->sp = regs;                           /* the saved stack pointer */
}

/* The time slice of task t, in ticks: its own if it has one, else QUANTUM. */
static uint32_t slice_of( const struct tcb *t )
{
    return t->slice != 0 ? t->slice : QUANTUM;
}

uint32_t task_start_first( void )
{
    /* Prepare every task: its variables, its first stack, its state. */
    for ( unsigned i = 0; i < NUM_TASKS; i++ )
    {
        partition_init( &tasks[i] );
        task_init_stack( &tasks[i] );
        tasks[i].state = TASK_READY;
    }

    /* The first task in the table runs first. */
    tcb_current = &tasks[0];
    tcb_current->state = TASK_RUNNING;
    rr_last = 0;
    quantum_left = slice_of( tcb_current );   /* the first task has a slice too */
    tick_count = 0;                         /* count from the start of the scheduler */
    kprintf( "scheduler: round robin, quantum %u ticks\n", QUANTUM );

    /* MPU region 1 = this task's partition, so the task may use it. */
    mpu_set_task_region( tcb_current->part_start,
                         tcb_current->part_end - tcb_current->part_start );

    /* The first task is entered by the return from this SVCall
     * exception, not by PendSV. That return pops only the 8-word
     * hardware frame, so PSP must point at the frame, 8 words above
     * the saved sp: the zeros for r4..r11 are simply skipped (a task's
     * first instruction does not care about r4..r11).
     * Then set nPRIV (bit 0 of CONTROL). This is done once, here: later
     * switches never touch CONTROL, so every task, idle included, runs
     * unprivileged. We are in handler mode, which is always privileged,
     * so nPRIV only takes effect when the exception returns to thread
     * mode, that is, in the task.
     * C equivalent (C cannot name PSP or CONTROL):
     *     PSP = tcb_current->sp + 8;
     *     CONTROL = CONTROL | 1;   // nPRIV = 1: thread mode is unprivileged
     */
    __asm volatile (
        "msr psp, %0        \n" /* PSP = the task's hardware frame */
        "mrs r1, control    \n" /* r1 = CONTROL (read) */
        "orr r1, r1, #1     \n" /* r1 |= 1: set nPRIV (modify) */
        "msr control, r1    \n" /* CONTROL = r1 (write) */
        "isb                \n" /* make the new CONTROL take effect now */
        : : "r" ( tcb_current->sp + 8 ) : "r1", "memory" );

    /* Return to thread mode on the process stack: the hardware pops the
     * frame from PSP, and the task starts at its entry with CONTROL = 3
     * (nPRIV = 1, and SPSEL = 1 set by this EXC_RETURN). */
    return EXC_RETURN_THREAD_PSP;
}

void task_request_switch( void )
{
    /* Writing 1 to ICSR.PENDSVSET makes PendSV pending. Writing 0 to
     * the other bits does nothing, so a plain store is enough. PendSV
     * has the lowest priority (0xE0, set in boot/startup.s): it runs
     * only when every other exception has finished, just before the
     * CPU would go back to the task. */
    SCB->ICSR = SCB_ICSR_PENDSVSET;
}

void task_increment_tick( void )
{
    tick_count++;                           /* one more tick */

    /* Before the first task starts there is nothing to schedule. */
    if ( tcb_current == 0 )
    {
        return;
    }

    /* Wake every application task whose delay is over. The test is
     * "wake_tick is now or in the past", written so that it stays
     * right when tick_count wraps from 0xFFFFFFFF to 0: the unsigned
     * difference is turned into a signed one, which is >= 0 when
     * tick_count has reached wake_tick (as long as the two are less
     * than 2^31 ticks apart, about 248 days). */
    int woke = 0;
    for ( unsigned i = 0; i < NUM_APP_TASKS; i++ )
    {
        if ( tasks[i].state == TASK_BLOCKED
             && ( int32_t ) ( tick_count - tasks[i].wake_tick ) >= 0 )
        {
            tasks[i].state = TASK_READY;    /* may run again */
            woke = 1;
        }
    }

    /* The idle task has no time slice: as soon as a task is READY again,
     * switch to it. */
    if ( tcb_current == IDLE_TCB )
    {
        if ( woke )
        {
            task_request_switch();
        }
        return;
    }

#if QUANTUM > 0
    /* An application task runs: one tick less in its time slice. When
     * the slice is used up, switch (preemption). */
    quantum_left--;
    if ( quantum_left == 0 )
    {
        task_request_switch();
    }
#endif
}

void task_block_until( uint32_t wake_tick )
{
    tcb_current->wake_tick = wake_tick;     /* when to wake up */
    tcb_current->state = TASK_BLOCKED;      /* not READY until then */
    task_request_switch();                  /* give the CPU to another task */
}

void task_finish_current( void )
{
    kprintf( "task %s exited (tick %u)\n", tcb_current->name, tick_count );
    tcb_current->state = TASK_FINISHED;     /* never runs again */

    /* Is any application task left that has not finished? */
    int left = 0;
    for ( unsigned i = 0; i < NUM_APP_TASKS; i++ )
    {
        if ( tasks[i].state != TASK_FINISHED )
        {
            left = 1;
        }
    }
    if ( !left )
    {
        /* Only the idle task is left: it runs from now on. */
        kprintf( "all tasks finished\n" );
    }

    task_request_switch();                  /* give the CPU to another task */
}

void task_switch_context( void )
{
    struct tcb *old = tcb_current;

    /* The old task stops running. If it is still RUNNING it was
     * preempted or yielded: it is READY for another turn. If it is
     * BLOCKED or FINISHED, it stays so. */
    if ( old->state == TASK_RUNNING )
    {
        old->state = TASK_READY;
    }

    /* Round robin: look at the application tasks in a circle, starting
     * just after the last one that ran, and take the first READY one.
     * The last one tried is the last task itself (k = NUM_APP_TASKS),
     * so a task that is the only READY one keeps the CPU. */
    struct tcb *next = IDLE_TCB;            /* if no task is READY: idle */
    for ( unsigned k = 1; k <= NUM_APP_TASKS; k++ )
    {
        unsigned i = ( rr_last + k ) % NUM_APP_TASKS;
        if ( tasks[i].state == TASK_READY )
        {
            next = &tasks[i];
            rr_last = i;                    /* the circle goes on from here */
            break;
        }
    }

    /* The chosen task becomes the current one, with a full time slice. */
    next->state = TASK_RUNNING;
    tcb_current = next;                     /* PendSV loads next->sp */
    quantum_left = slice_of( next );

    /* MPU region 1 = the new task's partition. From now on the old
     * task's partition is off limits, so isolation holds across the
     * switch. */
    mpu_set_task_region( next->part_start, next->part_end - next->part_start );

    /* Write the switch in the trace, but only if the task really
     * changed (a task that keeps the CPU is not a switch). The
     * pointer difference is the index in the task table. */
    if ( next != old )
    {
        trace_record_switch( old - tasks, next - tasks );
    }
}
