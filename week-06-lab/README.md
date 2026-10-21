# Week 6 laboratory — a task that starts again

This laboratory covers week 5's topic, scheduling, and extends the work
done in class: the operating system runs two tasks that take turns on
the processor, and a task that exits is finished for ever. Part A, in
the laboratory, changes what an exit means: a task can be marked to
start again from its entry point, with its stack and its variables as
at the first start. The first, natural attempt works in one demonstration
and fails in another; finding out why is the work. Part B, independent
practice, is a bank of short programming questions on the material of
the first five weeks. Hints for Part A are in [`HINTS.md`](HINTS.md),
worked solutions in [`SOLUTIONS.md`](SOLUTIONS.md), and the finished
kernel in [`solution/`](solution/).

## Objectives

On completion you will have:

- run an exit-and-restart that works in one case and not in another, and
  explained the difference from where the context switch saves the
  registers of the task that exits;
- made a task start again after an exit, with a new first stack and new
  variables, in the part of the kernel where that is safe;
- recorded the restarts in the scheduler's trace and, from the trace,
  stated a restarting task's cost and period and predicted its next
  restart;
- diagnosed a kernel that restarts a task with the variables of its
  previous run;
- (Part B) written, from a statement alone, functions with fixed
  signatures on topics of weeks 1 to 5, and compared each with a
  reference solution.

## Starting point

[`starter/`](starter/), a prepared copy of
[`../week-05/solution/`](../week-05/solution/), the finished week 5
operating system: tasks A and B, a round-robin scheduler with a time
slice of 5 ticks, an idle task, and the scheduler trace. Three things
have been added to it, and one taken out:

- `kernel/task.h` defines `enum task_exit_policy` (`EXIT_FINISH`,
  `EXIT_RESTART`) and the macro `B_EXIT_POLICY`, which is
  `EXIT_RESTART` in the demonstrations below and `EXIT_FINISH`
  otherwise. Nothing uses them yet: `struct tcb` has no field for the
  policy, and what a task does when it exits is not implemented.
- `kernel/trace.c` can record a restart (`trace_record_restart`) and
  prints each one as a line `R <tick> <task>` in the trace, before
  `TRACE END`. Nothing calls it yet.
- Three demonstrations, selected with `DEMO=<word>` as in week 5:
  `restart` (task B returns after three steps), `restart-deep` (task B
  ends with an explicit call to the exit system call, made three
  functions deep) and `restart-alone` (task A returns at once, so B has
  the processor to itself).

Work in `starter/`. Every path below is relative to it.

## Hints and solutions

Hints for Part A are in [`HINTS.md`](HINTS.md), three levels each, from
the concept to the shape of the code. Worked solutions to every exercise
are in [`SOLUTIONS.md`](SOLUTIONS.md), and the finished kernel is in
[`solution/`](solution/). You are expected to attempt each exercise
before consulting either: the failures you meet on the way are the
substance of the work, and a solution read in advance removes them.

## Structure

Part A is for the assisted session: its exercises build on one another.
Part B is for work you carry out independently afterwards; its
questions are independent of each other and of Part A.

Three conventions hold for every exercise:

- **State what you expect before you run.** Write down what the program
  should print, and when, if it is a restart. When the result differs,
  you have learned something about your model, not only about your code.
- **A demonstration that passes is not a proof.** Run every
  demonstration of an exercise, not only the first, and ask of each what
  it would have shown had your code been wrong.
- **Experiments are made in a copy.** A4 uses a defective file; make it
  in a copy of the finished kernel, so your working folder stays as the
  earlier exercises left it.

The demonstrations are built with `make DEMO=<word>`; each builds into
its own folder, `build/demo-<word>/`, so switching between them needs no
`make clean`:

```sh
make DEMO=restart
make DEMO=restart-deep
```

`make run DEMO=<word>` builds and runs one, and QEMU is stopped with
`Ctrl-C`. The kernel prints a line when a task exits and
when it restarts, with the tick, and `trace_dump()` prints the trace;
task B prints a `b_runs` value at every start, which is the evidence for
what happens to a restarting task's variables.

---

## Part A — in the laboratory

### A1. Exit means start again

Task B exits when its function returns, or when it calls the exit
system call. Since week 5, a task that exits is marked `TASK_FINISHED`
and is never chosen again. In this exercise, B is to start again
instead, and A is to keep finishing.

1. Build and run the starter with `make run DEMO=restart`. Record when
   task B exits and what the run ends with. Then write down what the
   run should print if B started again each time it exited.
2. Read `lecture-notes/week-05.md`, "Task states in the code" and
   "Building the first stack: `task_init_stack`", and
   `lecture-notes/week-04.md`, "`partition_init`: giving the variables
   their values". They say what the kernel does for a task before its
   first run. A task that starts again needs the same things.
3. Give `struct tcb` a field for the policy, set it in the task table
   (`B_EXIT_POLICY` for task B, `EXIT_FINISH` for the others), and
   change `task_finish_current` in `kernel/task.c` so that a task whose
   policy is `EXIT_RESTART` is made ready to run again from its entry,
   with its variables as at the first start. A task with `EXIT_FINISH`
   behaves as before.
4. Run `make run DEMO=restart`, then `make run DEMO=restart-deep`. After
   each, say what B printed after its first exit.

**Checkpoint:** you have run both demonstrations and can say what each
printed and which of the two shows that your code is correct.

<details><summary>Debrief</summary>

A first attempt that re-arms the task inside `task_finish_current`
passes `restart` and fails `restart-deep`.

In `restart` B prints `B: start, b_runs = 100` again after each exit, at
every restart. In `restart-deep` it prints that line once; after that,
`task B exited` appears again and again, and B never starts again.

The kernel does not switch tasks inside the exit system call. It makes
`PendSV` pending, and `PendSV_Handler` runs afterwards. Its first act is
to push `r4`–`r11` below the task's process stack pointer and to store
the result in the task's `sp`, the first field of its TCB. That store
replaces whatever the system call wrote there. The re-armed `sp` is
therefore kept only if the save happens to leave the same value, and it
does when the stack is empty: B returns from its entry function, so its
stack pointer is at the very top of the stack, the exit's saved frame
lies exactly where the new first frame is written, and the save stores
the same address. In `restart-deep` B calls the exit three functions
down, the stack pointer is lower, `PendSV` stores that lower address,
and B resumes just after its own `svc`, at the instruction that asks for
the exit again.

Instrumenting the two cases shows it: print `tcb_current->sp` at the end
of the re-arming and print the task's `sp` at the start of
`task_switch_context`. In `restart` the two values are equal; in
`restart-deep` the second is lower.

`restart` passed for a reason that has nothing to do with the code being
right. A test that passes by coincidence is the reason to run the second
demonstration.
</details>

### A2. Start again after the save

The re-arming must happen after `PendSV` has saved the registers of the
task that exited, and `task_switch_context` is the first kernel code
that runs after the save.

1. Instrument your A1 code as the debrief describes, run both
   demonstrations, and write the two pairs of values in a comment.
2. Move the re-arming out of `task_finish_current`. There, leave the
   task marked finished, as for any task. In `task_switch_context`, at
   the point where the task that stops is examined, start it again if it
   is finished and its policy is `EXIT_RESTART`.
3. Print `task <name> restarted (tick <n>)` when a task starts again
   (the exit message of `task_finish_current` is a model), and call
   `trace_record_restart( task index )`: the index is the task's
   position in the table.
4. Run the normal program, `restart`, `restart-deep` and `restart-alone`.
   Write down first what you expect of the last: A returns at once.

**Checkpoint:** in `restart` and `restart-deep`, B exits at tick 15 and
restarts at tick 15, A exits at tick 29, and B goes on restarting;
`b_runs = 100` is printed at every start. In `restart-alone`, A exits
at tick 0 and B restarts at tick 6, 12, 18 and so on. The normal program
prints exactly what the week 5 kernel printed. You can say why the
restart is now independent of how deep the exit was made.

<details><summary>Debrief</summary>

`task_switch_context` runs inside `PendSV_Handler` after the old task's
registers are already stored in its TCB, so the new first frame, built
by the same `task_init_stack` as for the first start, is the last write
to `sp` before the next task is loaded. The exit system call only has
to record the intent, which the state `TASK_FINISHED` and the policy
already say.

The variables come back because `partition_init` copies the initial
values from FLASH over the task's `.data` and `.bss`, as at the first
start: B's `b_runs` is 100 at every start, where without it the counter
would continue from the previous run.

A task that restarts is never finished, so the kernel must not report
`all tasks finished` while B restarts. In `restart-alone`, when B is the
only task that can run, the task that stops and the task that starts
are the same: the scan for the next task must find it again after it has
been made ready. This is why the re-arming comes before the scan.
</details>

### A3. Reading the restarts

With restarts the trace gains a second kind of line. In `DEMO=restart`,
task A ends its run with `trace_dump()`.

1. Run `make run DEMO=restart` and read the dump: the `T` lines (a
   switch: tick, from, to), the `R` line (a restart: tick, task) and the
   chart. Which lines does the dump hold for B's restart at tick 15?
2. Save the output of a run, up to the end of the dump, in a file, and
   give it to the course tool
   (`python3 ../../../course-material/tools/schedsim/schedsim.py trace <file>`).
   The tool reads the `T` lines and skips the `R` line.
3. After tick 29, B is alone. State B's cost, in ticks, from one run of
   it between two restarts, and its period, the ticks between two
   restarts. Predict the ticks of its next three restarts, then check
   them against the run: they are printed after the dump.

**Checkpoint:** you can say, from the output, B's cost and period when it
runs alone, and the next three restart ticks you predicted equal the
ones the run printed.

<details><summary>Debrief</summary>

After tick 29, B is the only task. One run of B is three steps of two
ticks, so its cost is 6 ticks, and it restarts at once when it exits,
so its period is also 6 ticks: it restarts at ticks 29, 35, 41, 47. When
A was still running the period was longer, because B was switched out
for A's time slices: B restarted at tick 15 and the next time at 29.

The `R` line is an addition of this laboratory: `schedsim trace` reads
only the `T` lines, so the chart it draws is the same as without the
restarts, and the week 5 kernel prints no `R` line.
</details>

### A4. A restart that keeps the variables

`broken/no-data-reset/task.c` is a replacement for `kernel/task.c`, with
a defect of one line. Copy `solution/` to a working folder, copy the
file over its counterpart, build, and run `make run DEMO=restart`.

Say what B prints at each start, find the line that is missing by
comparing the file with `solution/kernel/task.c`, and write in a comment
at that place what the line does and what it would have done at the
first start. Copying the missing line from `solution/` repairs the
program and is not the exercise.

**Checkpoint:** the symptom is described by the values `b_runs` takes,
and the missing line is named with the reason the counter goes on.

<details><summary>Debrief</summary>

B restarts at tick 15 as it should, but it prints `b_runs = 100`, then
101, 102, and so on. The new first stack is built, so B runs from its
entry, and the variables are not renewed: the call to `partition_init`
is missing from the restart, so `b_runs` keeps the value the previous
run left in B's partition.
</details>

### Done looks like — Part A

- The first attempt is run on both demonstrations and the difference is
  explained from where `PendSV` stores the task's `sp`.
- Task B restarts with fresh variables in `restart`, `restart-deep` and
  `restart-alone`; task A finishes as before; the normal program prints
  what the week 5 kernel printed.
- The restarts are in the trace, and you can state B's cost and period
  from it.
- The defective kernel is diagnosed at its line.

---

## Part B — independent practice

Eight short programming questions on the material of weeks 1 to 5, in
[`exam-practice/`](exam-practice/). Each question has its own folder
with a statement, the file you write, a program in which you try it, and
`make`; read [`exam-practice/README.md`](exam-practice/README.md) first.
The questions are independent of each other and of Part A: do them in
any order. Nothing is handed in, and nothing in the folders checks your
answers: the reference solution of each question is in
[`exam-practice/solutions/`](exam-practice/solutions/), to be read after
your own attempt. Deciding whether your code is right, by trying the
edges, is part of each question.

Six questions are about a function or a file of your own: B1 to B5 are
C functions with fixed signatures, B6 is a Makefile. B7 and B8 change
one function of the week 5 kernel, built from `week-05/solution/` with
your file.

### B1. Comparing ticks that wrap

[`exam-practice/q01-tick-before/`](exam-practice/q01-tick-before/):
telling which of two tick counts is the earlier when the counter wraps,
and how long there is still to wait.

### B2. A ring buffer

[`exam-practice/q02-ring-buffer/`](exam-practice/q02-ring-buffer/): put,
get, count and put-many on a ring of eight bytes, with the indices
anywhere.

### B3. Describing an MPU region

[`exam-practice/q03-mpu-size/`](exam-practice/q03-mpu-size/): the `SIZE`
field of a region, its attribute register, and whether a base address
is allowed.

### B4. The first stack of a task

[`exam-practice/q04-init-frame/`](exam-practice/q04-init-frame/): the
hardware frame, the saved context of a task that has never run, and the
saved program counter of one that is switched out.

### B5. Round robin on paper, in C

[`exam-practice/q05-rr-wait/`](exam-practice/q05-rr-wait/): the finish
times, the average waiting time and the number of switches of a round
robin schedule.

### B6. A Makefile that knows what depends on what

[`exam-practice/q06-makefile/`](exam-practice/q06-makefile/): header
dependencies for the objects, and a second program built from some of
the same objects.

### B7. Fixed priorities in the scheduler

[`exam-practice/q07-priority-pick/`](exam-practice/q07-priority-pick/):
the kernel chooses the READY task of highest priority, and keeps the
round robin among equals.

### B8. A time slice for each task

[`exam-practice/q08-time-slice/`](exam-practice/q08-time-slice/): the
kernel gives each task the time slice of its own.

### Done looks like — Part B

- You have written, from the statement alone, the functions or files of
  the questions you chose, and tried each one on the edges as well as on
  the examples.
- You have read the reference solution of each, after your attempt, and
  can say where it differs from yours and whether that matters.

---

## Where this goes next

The next lecture treats scheduling of tasks that repeat, with periods and
deadlines. This laboratory has prepared two things it uses.

- **A task that repeats is a job with a cost and a period.** A restarting
  task starts again each time it exits. In A3 you stated, from the
  trace, how many ticks one run costs and how many ticks pass between
  two starts, and predicted the next start: those two numbers, per task,
  are what the analysis of a set of repeating tasks begins with.
- **A kernel whose behaviour you can predict.** The kernel counts the work
  of a task in ticks, so its trace is the same at every run, and what you
  predict from the cost and period of the tasks can be checked tick by
  tick against what the kernel did.
