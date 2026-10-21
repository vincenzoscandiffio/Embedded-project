# Week 6 laboratory — hints

Hints for the exercises of [`README.md`](README.md), to be read when an
exercise does not yield to your own attempt and before its worked
solution in [`SOLUTIONS.md`](SOLUTIONS.md). Each exercise has three
levels. Open one at a time, and stop at the first that lets you
continue: every level is complete in itself, and none contains the
finished code.

1. **The concept** — what to think about, without naming a function.
2. **The rule** — the function, field or rule involved.
3. **The shape** — the order of the steps, as an outline to fill in.

The questions of Part B have no hints: each statement is complete, and a
reference solution follows the attempt.

---

## Part A

### A1. Exit means start again

<details><summary>Level 1 — the concept</summary>

Start from the first start. Before a task runs for the first time, the
kernel gives it three things: values for its variables, a stack that
looks as if the task had already run and been switched out, and a state
that lets the scheduler choose it. A task that starts again is in the
same position as a task that never started, so it needs the same three
things again.

Then ask where the kernel learns that a task has exited, and whether that
is also the place where it is safe to change what the task will find on
its stack when it next runs. The second question is the one the first
attempt answers by assuming, and the demonstrations are there to test it.
</details>

<details><summary>Level 2 — the rule</summary>

`kernel/task.c` already has a function for each of the first two things:
one copies the initial values of a task's variables from FLASH into its
partition, and one builds the first stack and stores its address in the
task's `sp`. `task_start_first` calls both for every task. The third is
the task's state field.

The exit system call ends in `task_finish_current`, which runs in the
SVCall exception on behalf of the task that exits. It already prints the
exit message and marks the task finished. The policy is a new field of
`struct tcb`, which must not come before `sp`: the context switch is
written in assembly and reads `sp` at offset 0.
</details>

<details><summary>Level 3 — the shape</summary>

In `task_finish_current`:

- print the exit message, as now;
- if the policy of the task that exits is restart: renew its variables,
  build its first stack again, mark it ready, ask for a switch;
- otherwise: mark it finished, as now.

The test of the last task left finished must not count a task that
restarts.
</details>

### A2. Start again after the save

<details><summary>Level 1 — the concept</summary>

Draw the order of events from the `svc` in the task to the first
instruction of the next task: the system call, the handler that runs it,
the exception that does the switch. Mark each place where the task's `sp`
is written. If two writes to the same variable are in the wrong order,
the later one wins, and the earlier one is lost.
</details>

<details><summary>Level 2 — the rule</summary>

`PendSV_Handler` begins by pushing `r4`–`r11` onto the process stack and
storing the new stack pointer in the exited task's `sp`, and only then
calls `task_switch_context`. So `task_switch_context` is the first
kernel function that runs after that store. It looks at the task that is
stopping before it chooses the next one, and it is also where a task
that was `RUNNING` becomes `READY`.

A restart there must come before the scan for the next task: when the
restarting task is the only one that can run, the scan has to find it.
</details>

<details><summary>Level 3 — the shape</summary>

In `task_switch_context`, where the old task's state is examined:

- if it was running: it becomes ready, as now;
- otherwise, if it is finished and its policy is restart: renew its
  variables, build its first stack, mark it ready, print the restart
  message with the tick, record the restart in the trace;
- then the scan for the next task, unchanged.

In `task_finish_current`, only the exit message and the finished state
are left.
</details>

### A3. Reading the restarts

<details><summary>Level 1 — the concept</summary>

A task that restarts is a job that repeats. Two numbers describe it:
how long it works each time, and how long passes between the starts. They
are equal only if nothing else uses the processor between two runs.
</details>

<details><summary>Level 2 — the rule</summary>

The `R` lines give the ticks of the restarts. The difference between two
consecutive `R` lines of the same task is its period. The cost is the
number of ticks the task owns the processor between its start and its
exit, which the `T` lines or the chart show; when the task is alone,
cost and period are the same number.
</details>

<details><summary>Level 3 — the shape</summary>

List the `R` ticks of B in order. Take the differences. Take the last
tick and add the difference that repeats once B is alone. Do it three
times. Then compare with the `task B restarted` lines printed after the
dump.
</details>

### A4. A restart that keeps the variables

<details><summary>Level 1 — the concept</summary>

Look at the values `b_runs` takes, not at the lines that say B restarted.
A restart that gives B a fresh stack but not fresh variables looks like a
restart from the outside. Ask what of the first start is missing.
</details>

<details><summary>Level 2 — the rule</summary>

Compare the two files with `diff`. The difference is one line, in the
function that starts a task again. That line is also called once for
every task, when the scheduler starts.
</details>

<details><summary>Level 3 — the shape</summary>

The function that starts a task again does five things in order: the
variables, the stack, the state, a message, a trace record. Find the one
that the defective file does not do, and say in one sentence what the
counter would be if that thing were done.
</details>
