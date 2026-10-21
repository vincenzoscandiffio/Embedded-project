# Q6. A Makefile that knows what depends on what

The folder holds a small program for the Cortex-M3: four sources in
`src/` and two headers in `include/`. Its `Makefile` already builds the
program `build/prog.axf` from `main.c`, `led.c` and `util.c`, with the
objects in `build/`. Its answer is the `Makefile` itself: you change it,
and you do not change the sources.

The `Makefile` has two shortcomings.

## What you write

**(a)** `make` does not know about the headers. After `include/util.h`
is edited, `make` says there is nothing to do, and the program keeps the
old code. Change the `Makefile` so that an object is built again exactly
when a source it depends on changes: its own `.c` file, or a header that
the `.c` file includes. That holds for every object the `Makefile` can
build, including the one for `src/selftest.c`. An object that does not
use the edited header is not built again. To decide which objects depend
on which header, read the sources.

**(b)** `src/selftest.c` is the `main` of a second program, a check of
`add`. Add to the `Makefile` the second program, `build/check.axf`: it is
linked, with the same compiler options as `build/prog.axf`, from the
objects of `led.c`, `util.c` and `selftest.c`, and not from the object of
`main.c`, since two `main` would clash. It is linked again whenever one of
its objects is newer. A plain `make` still builds only `build/prog.axf`.

Nothing else changes: the target `clean` still removes `build/`.

## Example

After `make`, if only `src/util.c` is edited, `make` compiles `util.c`
again and links `build/prog.axf` again, and does not compile `main.c` or
`led.c`.
