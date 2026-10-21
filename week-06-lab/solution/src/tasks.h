/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * The tasks: the code that runs unprivileged, after the kernel has
 * started. A and B take turns on the CPU (round robin); the idle task
 * runs when neither can.
 */
#ifndef TASKS_H
#define TASKS_H

/* Task A: prints a numbered step after each piece of work, then
 * returns (= exit). */
void task_a( void );

/* Task B: the same as A, with more steps, so it keeps running alone
 * after A has finished. */
void task_b( void );

/* The idle task: sleeps (`wfi`) for ever. Never returns. */
void idle_task( void );

#endif /* TASKS_H */
