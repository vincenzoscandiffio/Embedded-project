/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Tasks and the scheduler, kernel side: the task control blocks, the
 * task table, and the code that decides which task runs next.
 *
 * A task always runs unprivileged, in thread mode, on its own stack
 * (the process stack, PSP), inside its own RAM partition. Since week 5
 * the tasks take turns on the CPU (preemptive round robin):
 *   - SysTick counts the ticks and, when the running task has used up
 *     its time slice (QUANTUM ticks), asks for a switch;
 *   - a task can also give the CPU away itself: yield, delay, exit;
 *   - the switch itself happens in PendSV (kernel/pendsv.s), which
 *     calls task_switch_context() to choose the next task.
 * When no application task can run, the idle task runs.
 */
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

/* The length of a time slice, in ticks (one tick = 10 ms). A task that
 * keeps the CPU for QUANTUM ticks in a row is switched out. Change it
 * from the command line: `make QUANTUM=2 run`. QUANTUM = 0 means "never
 * switch on the tick": tasks run until they give the CPU away
 * themselves (cooperative scheduling, `make DEMO=coop`). */
#ifndef QUANTUM
#define QUANTUM 5
#endif

/* The states of a task. */
enum task_state {
    TASK_READY,         /* could run, waits for its turn */
    TASK_RUNNING,       /* owns the CPU now (exactly one task at a time) */
    TASK_BLOCKED,       /* waits for a tick (task_delay, task_delay_until) */
    TASK_FINISHED,      /* has called exit: never runs again */
};

/* The task control block (TCB): everything the kernel knows about one
 * task. The addresses come from the linker script (scripts/mps2_m3.ld). */
struct tcb {
    /* The task's saved stack pointer, while the task is not running.
     * It MUST stay the first field: PendSV_Handler (kernel/pendsv.s) is
     * assembly and reads and writes it at offset 0 of the TCB, with a
     * plain `ldr`/`str`, without knowing the rest of the struct. */
    uint32_t *sp;
    const char *name;           /* short name for messages, e.g. "A" */
    void ( *entry )( void );    /* the function the task runs */
    uint32_t *stack_top;        /* first address above the task's stack */
    uint32_t part_start;        /* lowest address of the partition */
    uint32_t part_end;          /* first address after the partition */
    uint32_t *data_start;       /* the task's variables in RAM: start... */
    uint32_t *data_end;         /* ...and end (exclusive) */
    const uint32_t *data_load;  /* their initial values, stored in FLASH */
    enum task_state state;      /* READY, RUNNING, BLOCKED or FINISHED */
    uint32_t wake_tick;         /* if BLOCKED: the tick at which it is READY again */
    uint8_t priority;           /* Q7: a higher number is a higher priority */
};

/* Q7: the priorities of tasks A and B. A higher number means a higher
 * priority (the course rule). Change them to try other cases. */
#ifndef A_PRIO
#define A_PRIO 1
#endif
#ifndef B_PRIO
#define B_PRIO 1
#endif

/* How many tasks the table has: A, B and the idle task. */
#define NUM_TASKS 3

/* The task table (defined in kernel/task.c). The trace reads the names. */
extern struct tcb tasks[NUM_TASKS];

/* The TCB of the task that runs now, or 0 while the kernel runs alone
 * (before the first task starts). PendSV_Handler reads and writes it. */
extern struct tcb *tcb_current;

/* Ticks since the scheduler started (one tick = 10 ms). */
extern uint32_t tick_count;

/* Kernel-only service: prepare every task and start the first one.
 * Called from svc_dispatch when main() asks with `svc`. Returns the
 * EXC_RETURN value that enters the task (0xFFFFFFFD). */
uint32_t task_start_first( void );

/* Called by SysTick_Handler at every tick: count the tick, wake the
 * tasks whose delay is over, and ask for a switch when the time slice
 * of the running task is used up. */
void task_increment_tick( void );

/* Ask for a context switch: make PendSV pending. The switch happens as
 * soon as no more urgent exception is running. */
void task_request_switch( void );

/* Block the current task until tick_count reaches wake_tick, and ask
 * for a switch. Used by the delay system calls. */
void task_block_until( uint32_t wake_tick );

/* The current task has called exit: mark it finished, and ask for a
 * switch. It never runs again. */
void task_finish_current( void );

/* Called only by PendSV_Handler (kernel/pendsv.s), after it has saved
 * the old task's registers: choose the next task (round robin) and make
 * it the current one (tcb_current, states, MPU region 1). */
void task_switch_context( void );

#endif /* TASK_H */
