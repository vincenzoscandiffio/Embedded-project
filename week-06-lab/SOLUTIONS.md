# Week 6 laboratory — worked solutions

Solutions to every exercise are provided here. You are expected to
attempt an exercise before consulting its solution, and to read the
hints in [`HINTS.md`](HINTS.md) before this file: the values an
exercise asks you to observe are the substance of the work, and reading
an observed value does not substitute for taking one.

Where an exercise leaves the code to you, its solution follows the
course of the work: the first attempt that the exercise text leads to,
what it does, how the cause is found, and then the working code. The
symptoms were produced by running those attempts.

Task document: [`README.md`](README.md). Finished code of Part A:
[`solution/`](solution/), the kernel after A2 (A3 reads its output). The
defective kernel of A4 is in [`broken/`](broken/), one replacement file.
The reference solutions of the questions of Part B follow Part A.

Every value, output and listing below was produced by executing the
stated command using the course toolchain (Arm GNU Toolchain 15.3,
QEMU 11.1): against `starter/` with the stated code, or against
`solution/`, or, for the defective kernel, against `solution/` with the
stated file. Each observed value is also classified by what fixes it:

- **architectural** — the ARMv7-M architecture defines it; any
  Cortex-M3 produces it;
- **this build** — it depends on the code the compiler generated, on the
  linker's placement or on how the emulator behaves, so your value may
  differ; the relation the exercise asks about holds regardless.

Tick numbers are counted from the scheduler's start and depend only on
the work the tasks do, so they are the same on every run (this build).

---

## Part A

### A1. Exit means start again

**The starter's first run.** `make run DEMO=restart` prints, among the
lines of the two tasks,

```text
task B exited (tick 15)
A: step 6 (tick 16)
...
TRACE END 24
...
task A exited (tick 24)
all tasks finished
```

B exits at tick 15 and never starts again; A finishes at tick 24 and the
kernel reports that nothing is left (this build).

**The first attempt.** The natural place is `task_finish_current`, where
the kernel learns of the exit. With a new field `exit_policy` at the end
of `struct tcb` (after `wake_tick`, so `sp` stays first), set to
`B_EXIT_POLICY` in B's entry of the table, the attempt is:

```c
if ( tcb_current->exit_policy == EXIT_RESTART )
{
    partition_init( tcb_current );          /* variables as at the first start */
    task_init_stack( tcb_current );         /* a new first frame, sets sp */
    tcb_current->state = TASK_READY;
    task_request_switch();
    return;
}
tcb_current->state = TASK_FINISHED;
```

placed after the exit message. Its two runs differ.

`make run DEMO=restart`: B prints `B: start, b_runs = 100` again after
every exit, as the task document asks. The restarts happen; the
variables are fresh.

`make run DEMO=restart-deep`: `B: start` is printed once. After tick 15,
`task B exited` is printed again, at every time slice of B, and B never
prints a second start (this build).

**Finding the cause.** The difference between the two demonstrations is
how deep in its stack B is when it asks to exit. Print `tcb_current->sp`
at the end of the re-arming, and the task's `sp` at the start of
`task_switch_context`, in both. In `restart` the two values are equal,
`0x200013c0`, which is `_task_b_stack_top` minus 64 bytes: the 8 words of
the hardware frame and the 8 of `r4`–`r11` of a first stack (this
build). In `restart-deep` the first is `0x200013c0` and the second is
lower (`0x20001338` in the instrumented build, this build). Something
wrote `sp` between the two points, and `PendSV_Handler` does:
`stmdb r0!, {r4-r11}` and then `str r0, [r2]` with `r2 = tcb_current`.

In `restart`, B has returned from its entry function: its stack is
empty, the stack pointer is the top of the stack, and the system call's
8-word frame is pushed 32 bytes below it, exactly where
`task_init_stack` has written the new first frame. `PendSV` then pushes
`r4`–`r11` 32 bytes lower still, where the new first stack has its own
`r4`–`r11` words, and stores that address, which is the one the
re-arming stored. Everything agrees by coincidence of the stack
geometry (architectural for the stacking and the stores; this build for
the addresses).

In `restart-deep` the stack pointer is lower, so the system call's frame
and `PendSV`'s save are lower too. `PendSV` stores that lower address
over the new one, and B resumes at the `b sys_exit` after its own
`svc`, asks to exit again, is re-armed again, and so on.

### A2. Start again after the save

The re-arming moves to `task_switch_context`. `task_finish_current`
keeps only the exit message and the finished state (and the test of
whether a task is left, which must not count a task that restarts):

```c
tcb_current->state = TASK_FINISHED;
/* ... */
if ( tasks[i].state != TASK_FINISHED
     || tasks[i].exit_policy == EXIT_RESTART )
{
    left = 1;
}
```

and a function that does the work, in `kernel/task.c` after
`task_init_stack`:

```c
static void task_restart( struct tcb *t )
{
    partition_init( t );                    /* variables as at the first start */
    task_init_stack( t );                   /* a new first frame, sets t->sp */
    t->state = TASK_READY;                  /* competes for the CPU again */
    kprintf( "task %s restarted (tick %u)\n", t->name, tick_count );
    trace_record_restart( t - tasks );      /* one `R` line in the dump */
}
```

called from `task_switch_context` where the old task's state is
examined, before the scan:

```c
if ( old->state == TASK_RUNNING )
{
    old->state = TASK_READY;
}
else if ( old->state == TASK_FINISHED && old->exit_policy == EXIT_RESTART )
{
    task_restart( old );
}
```

Outputs, all from `solution/`:

- `DEMO=restart` and `DEMO=restart-deep`: `task B exited (tick 15)`,
  `task B restarted (tick 15)`, `B: start, b_runs = 100` at every start;
  A exits at tick 29; B restarts at ticks 29, 35, 41, 47, ...; the
  kernel never prints `all tasks finished`.
- `DEMO=restart-alone`: `task A exited (tick 0)`, then B starts and
  restarts at ticks 6, 12, 18, ... In that run the task that stops and
  the task that starts are the same one, and the scan finds it because
  the restart came first.
- The normal program: the same 46 lines as the week 5 kernel, ending
  with `TRACE END 44` and `all tasks finished`.

The `kprintf` inside `PendSV` works as the one inside `SVCall` did: it
polls the UART and needs no interrupt.

### A3. Reading the restarts

The dump of `DEMO=restart`, as task A prints it at tick 29:

```text
TRACE BEGIN
T 5 A B
T 10 B A
T 15 A B
T 15 B A
T 20 A B
T 25 B A
R 15 B
TRACE END 29
```

B's restart at tick 15 is three lines: B starts at 15 (`T 15 A B`), exits
at once (`T 15 B A`), and the restart is `R 15 B`. The tool reads the
`T` lines:

```text
trace: ticks 5..29 (24 ticks, 6 switches); time 0 = tick 5
time   0    5    10   15   20
       |....|....|....|....|....
A      .....##########.....####
B      #####..........#####....
CPU    BBBBBAAAAAAAAAABBBBBAAAA
schedule=BBBBBAAAAAAAAAABBBBBAAAA
```

and skips the `R` line.

After tick 29 B is alone. One run is three steps of two ticks: cost
6 ticks. It restarts when it exits, so the period is 6 ticks too, and
the next three restarts after tick 29 are 35, 41 and 47, as printed
(`task B restarted (tick 35)`, `(tick 41)`, `(tick 47)`). Before, A took
turns: B restarted at tick 15 and next at 29, a period of 14 ticks.

### A4. A restart that keeps the variables

`broken/no-data-reset/task.c` is `solution/kernel/task.c` without the
line `partition_init( t );` in `task_restart`. With it in place,
`make run DEMO=restart` prints:

```text
B: start, b_runs = 100
...
task B restarted (tick 15)
B: start, b_runs = 101
...
task B restarted (tick 29)
B: start, b_runs = 102
```

and so on, one more at every start. B restarts, from its entry, with a
fresh stack. Its variable `b_runs` is in B's `.data`, and the only code
that writes B's initial values into its partition is `partition_init`.
Without the call, the restart leaves in the partition what the previous
run left, and `b_runs++` at the end of each run adds up.

`diff solution/kernel/task.c broken/no-data-reset/task.c` shows the one
line.

---

## Part B

Each question has one reference solution, a file that meets its statement. The
files carry comments that say why each line is there. Read one after your own
attempt, and compare it with yours against the statement: a solution that
differs from the reference is not wrong for that reason alone.

### B1. Comparing ticks that wrap

The reference solution is [`exam-practice/solutions/q01-tick-before/answer.c`](exam-practice/solutions/q01-tick-before/answer.c).

### B2. A ring buffer

The reference solution is [`exam-practice/solutions/q02-ring-buffer/answer.c`](exam-practice/solutions/q02-ring-buffer/answer.c).

### B3. Describing an MPU region

The reference solution is [`exam-practice/solutions/q03-mpu-size/answer.c`](exam-practice/solutions/q03-mpu-size/answer.c).

### B4. The first stack of a task

The reference solution is [`exam-practice/solutions/q04-init-frame/answer.c`](exam-practice/solutions/q04-init-frame/answer.c).

### B5. Round robin on paper, in C

The reference solution is [`exam-practice/solutions/q05-rr-wait/answer.c`](exam-practice/solutions/q05-rr-wait/answer.c).

### B6. A Makefile that knows what depends on what

The reference solution is [`exam-practice/solutions/q06-makefile/Makefile`](exam-practice/solutions/q06-makefile/Makefile).

### B7. Fixed priorities in the scheduler

The reference solution is [`exam-practice/solutions/q07-priority-pick/kernel/task.c`](exam-practice/solutions/q07-priority-pick/kernel/task.c).

### B8. A time slice for each task

The reference solution is [`exam-practice/solutions/q08-time-slice/kernel/task.c`](exam-practice/solutions/q08-time-slice/kernel/task.c).
