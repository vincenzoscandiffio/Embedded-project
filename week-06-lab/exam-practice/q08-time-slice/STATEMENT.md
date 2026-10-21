# Q8. A time slice for each task

In the round-robin kernel, every task that gets the processor keeps it for
`QUANTUM` ticks (5 by default) unless it gives it away sooner. In this
question, each task may have its own slice.

This folder holds two files of the kernel.

- `kernel/task.h` is given. Every task control block now has a `slice`,
  a number of ticks, and the file defines `A_SLICE` and `B_SLICE`, the
  slices of tasks A and B (both 0). The value 0
  means "no slice of its own": the task gets `QUANTUM` ticks.
- `kernel/task.c` is your answer. It is the round-robin version: it still
  gives `QUANTUM` ticks to every task. You change how long the time slice
  is, and nothing else.

## What you write

In `kernel/task.c`, after your change:

**(a)** When the scheduler switches to a task, the task's time slice is
its `slice` ticks, or `QUANTUM` ticks if its `slice` is 0.

**(b)** The same holds for the first task that the kernel starts, before
any switch.

Nothing else changes: which task is chosen, and when a task is switched
out other than at the end of its slice.

## Example

With `A_SLICE` 4 and `B_SLICE` 2, the first lines of the trace are

```text
T 4 A B
T 6 B A
T 10 A B
```

Task A runs 4 ticks, then task B 2 ticks, then task A 4 ticks again.
