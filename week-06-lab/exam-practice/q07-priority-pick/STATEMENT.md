# Q7. Fixed priorities in the scheduler

The kernel is a preemptive round-robin kernel: tasks A and B take turns
with a time slice of 5 ticks, and the idle task runs when neither can.
This folder holds two of its files.

- `kernel/task.h` is given. Every task control block now has a
  `priority`, a number, and the file defines `A_PRIO` and `B_PRIO`, the
  priorities of tasks A and B (both 1). The idle task has priority 0 and is not a candidate.
- `kernel/task.c` is your answer. It is the round-robin version: it still
  chooses the next task by round robin. You change one thing in it: the
  choice of the next task, in `task_switch_context()`.

The rule is: **a higher number is a higher priority**.

## What you write

In `task_switch_context()`, the scan that chooses the next task. After
your change:

**(a)** Of the application tasks that are READY, the kernel chooses the
one with the highest priority.

**(b)** Among READY tasks with the same priority, the turn goes round
robin, as it does now: the scan starts just after the task that ran
last. With `A_PRIO` equal to `B_PRIO`, the kernel behaves exactly as
the round-robin kernel does now.

**(c)** A task that is BLOCKED or FINISHED is never chosen, whatever its
priority. When no application task is READY, the idle task runs.

Nothing else changes. In particular, the kernel still switches only
when the time slice ends or the running task yields, blocks or exits: a
task of higher priority that becomes READY waits for the next switch.

## Example

With `A_PRIO` 2 and `B_PRIO` 1, task A is READY and has the higher
priority at every switch, so it is never switched out for B: it runs
its ten steps one after the other and exits at tick 20. Task B starts
only then.
