# Q5. Round robin on paper, in C

Several tasks are ready at tick 0, in the order of the array: task 0
first. The scheduler is round robin with a time slice of `q` ticks:
the task at the head of the queue runs for `q` ticks, or until it has
used its whole burst if that is shorter, and then, if it is not
finished, goes to the end of the queue. The next task starts at once;
a task that finishes in the middle of a slice ends the slice early. A
task needs `burst[i]` ticks of the processor to finish.

For a task, the finish time `f` is the tick at which it finishes. Its
waiting time is `f - burst[i]`, since it arrived at tick 0: the ticks
spent ready and not running.

There are at most `RR_MAX_TASKS` = 8 tasks. Every burst is at least 1,
and `q` is at least 1.

## What you write

In `answer.c`, three functions. Keep their signatures.

**(a)** `uint32_t rr_finish_time( const uint32_t *burst, unsigned n, unsigned q, unsigned i )`

Returns the finish time of task `i`, for the `n` tasks with bursts
`burst[0]` to `burst[n - 1]` and the time slice `q`.

**(b)** `uint32_t rr_avg_wait_x100( const uint32_t *burst, unsigned n, unsigned q )`

Returns the average waiting time of the `n` tasks, multiplied by 100 and
truncated to an integer (an average of 4.333... is returned as 433). Use
`rr_finish_time`.

**(c)** `unsigned rr_switches( const uint32_t *burst, unsigned n, unsigned q )`

Returns the number of context switches in the whole schedule: how many
times the processor goes from one task to a different one. A task that
runs two slices in a row, because it is the only one left, is not a
switch.

## Examples

These show the meaning of the functions. They are not all the cases. In
the table, `( uint32_t[] ){ 5, 3, 1 }` is an array of three bursts. The
schedule of these three tasks with `q` = 2 is A A B B C A A B A, where
A, B and C are tasks 0, 1 and 2.

| call | result |
|---|---|
| `rr_finish_time( ( uint32_t[] ){ 5, 3, 1 }, 3, 2, 0 )` | 9 |
| `rr_finish_time( ( uint32_t[] ){ 5, 3, 1 }, 3, 2, 1 )` | 8 |
| `rr_finish_time( ( uint32_t[] ){ 5, 3, 1 }, 3, 2, 2 )` | 5 |
| `rr_avg_wait_x100( ( uint32_t[] ){ 5, 3, 1 }, 3, 2 )` | 433 |
| `rr_switches( ( uint32_t[] ){ 5, 3, 1 }, 3, 2 )` | 5 |
