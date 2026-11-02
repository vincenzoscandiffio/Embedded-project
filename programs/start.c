/*
 * Loadable-program startup: the ELF entry point every program in
 * programs/ links to (programs/programs.ld sets ENTRY(_start)).
 *
 * Unlike boot/startup.s, this never touches privilege, the vector
 * table, or .data/.bss: by the time this task first runs, the kernel's
 * loader has already copied code and data into the slot and zeroed
 * BSS (R6), and the given task scaffolding has already built the
 * initial exception frame with this program's own stack (week 5's
 * task_init_stack, kernel/task.c). _start's only job is R1's contract:
 * call main, and make a return from main act exactly like calling
 * sys_exit(). That does not happen on its own -- `bl main` overwrites
 * lr, so simply falling off the end of _start would not reach
 * sys_exit by itself.
 */
#include "lib/syscalls.h"

int main( void );

void _start( void )
{
    main();
    sys_exit();
}
