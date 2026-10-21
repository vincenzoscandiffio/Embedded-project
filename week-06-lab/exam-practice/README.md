# Practice questions

Short programming questions on the material of weeks 1 to 5. Each one is
a folder, `qNN-<topic>/`, with a `STATEMENT.md` that says what is
required, and the file or files you write:

| Question | You write |
|---|---|
| Q1 to Q5 | `answer.c`, which already holds the types and a stub for every function |
| Q6 | the `Makefile` of the folder, which builds the sources in `src/` and `include/` |
| Q7, Q8 | `kernel/task.c` of the week 5 kernel, next to the given `kernel/task.h` |

Nothing else in a question folder is needed to answer it.

## Trying your code

`try/` is a convenience for running what you wrote, outside the
questions: nothing in it checks an answer, and deciding whether your code
is right is part of the question. Test the edges, such as the smallest and
the largest values and the wrap of a counter.

From `try/`:

- `make run Q=q02` (Q1 to Q5) builds your `answer.c` with `play/q02.c`, a
  `main` that includes your file and calls your functions, and runs it in
  QEMU. Edit `play/q02.c` to call the functions with the values you
  choose and print the results with `kprintf`. Stop it with `Ctrl-C`.
  `runtime/` holds what such a program needs: a start-up that prepares
  memory as the boot code of week 2 does, a polled console on UART0 with
  the `kprintf` of week 3, and a linker script with the memory map you
  know. The program runs alone, privileged: there is no kernel and no
  task.
- `make run Q=q07` (Q7, Q8) builds the kernel of
  `../../week-05/solution/` with the two files of the question's
  `kernel/` laid over it, and runs it in QEMU, as in week 5. As there,
  `DEMO=idle` selects a demonstration.
- Q6 is run in its own folder: `make`, `make -n` and `make clean`.

## After your attempt

`solutions/qNN-<topic>/` holds a reference solution that meets the
statement: the file you write, with the same name. Read it after your
own attempt, and compare: a solution that differs from yours is not wrong
for that reason alone, so check it against the statement and the cases
you tried.
