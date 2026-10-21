# Q4. The first stack of a task

When the kernel switches to a task, it loads the task's saved stack
pointer, pops `r4`–`r11` itself, and returns from the exception: the
processor pops the 8 words of the hardware frame. A task that has never
run has no saved registers, so the kernel writes on its stack what a
switch would have left there.

The stack grows down. A saved context is 16 words: the 8 words of the
hardware frame at the top, and below them the 8 words for `r4`–`r11`.
The saved stack pointer points at the lowest word. In words, counting
from the lowest address of the hardware frame:

| word of the hardware frame | register | value for a new task |
|---|---|---|
| 0 | r0 | 0 |
| 1 | r1 | 0 |
| 2 | r2 | 0 |
| 3 | r3 | 0 |
| 4 | r12 | 0 |
| 5 | lr | the address of the exit function, as it is |
| 6 | pc | the address of the entry function with bit 0 cleared |
| 7 | xPSR | `0x01000000` (only the T bit) |

Below the hardware frame, the 8 words of `r4`–`r11` are 0 for a new
task. Addresses of functions are `uint32_t` values: write
`(uint32_t) entry`.

## What you write

In `answer.c`, three functions. Keep their signatures.

**(a)** `void frame_hw( uint32_t *frame, void ( *entry )( void ), void ( *exit_fn )( void ) )`

Writes the 8 words of the hardware frame at `frame[0]` to `frame[7]`, as
in the table, and nothing else.

**(b)** `uint32_t *frame_init( uint32_t *stack_top, void ( *entry )( void ), void ( *exit_fn )( void ) )`

`stack_top` is the first address above the task's stack. Writes the whole
first context of a new task just below it: the hardware frame at the top
(use `frame_hw`) and the 8 words of `r4`–`r11` below it, and returns the
saved stack pointer. It writes nothing else.

**(c)** `void frame_set_pc( uint32_t *saved_sp, void ( *new_entry )( void ) )`

`saved_sp` is the saved stack pointer of a task that has been switched
out, so it points at the saved `r4`. Changes the saved `pc` of that task
to `new_entry` with bit 0 cleared, and nothing else: the task will
resume there.

## Examples

These show the meaning of the functions. They are not all the cases.
`frame_init` is called with a stack top at `0x20100000` and an exit
function at address 0.

| call | result |
|---|---|
| `( uintptr_t ) 0x20100000 - ( uintptr_t ) frame_init( ( uint32_t * ) 0x20100000, 0, 0 )` | 64 |
| `frame_init( ( uint32_t * ) 0x20100000, 0, 0 )[ 15 ]` | 0x01000000 |
| `frame_init( ( uint32_t * ) 0x20100000, ( void ( * )( void ) ) 0x08001001, 0 )[ 14 ]` | 0x08001000 |
| `frame_init( ( uint32_t * ) 0x20100000, 0, 0 )[ 3 ]` | 0 |
