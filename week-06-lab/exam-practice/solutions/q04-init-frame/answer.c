/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q4: the reference solution.
 */

#include <stdint.h>

/* xPSR with only bit 24 (T, Thumb state) set. */
#define XPSR_THUMB 0x01000000u

/* (a) The 8 words the processor pops on exception return, lowest
 * address first: r0, r1, r2, r3, r12, lr, pc, xPSR. */
void frame_hw( uint32_t *frame, void ( *entry )( void ), void ( *exit_fn )( void ) )
{
    frame[0] = 0;                               /* r0 */
    frame[1] = 0;                               /* r1 */
    frame[2] = 0;                               /* r2 */
    frame[3] = 0;                               /* r3 */
    frame[4] = 0;                               /* r12 */
    frame[5] = ( uint32_t ) exit_fn;            /* lr: where the task goes when it returns */
    frame[6] = ( uint32_t ) entry & ~1u;        /* pc: the entry, bit 0 ("Thumb") cleared */
    frame[7] = XPSR_THUMB;                      /* xPSR: only the T bit */
}

/* (b) 8 words below the top for the hardware frame, 8 more below them
 * for r4-r11 (zero), and the saved sp is the lowest of the 16. */
uint32_t *frame_init( uint32_t *stack_top, void ( *entry )( void ), void ( *exit_fn )( void ) )
{
    uint32_t *frame = stack_top - 8;
    frame_hw( frame, entry, exit_fn );

    uint32_t *regs = frame - 8;
    for ( int i = 0; i < 8; i++ )
    {
        regs[i] = 0;                            /* r4 + i */
    }
    return regs;
}

/* (c) saved_sp points at the saved r4: 8 words of r4-r11, then the
 * hardware frame, whose word 6 is the pc: word 8 + 6 = 14. */
void frame_set_pc( uint32_t *saved_sp, void ( *new_entry )( void ) )
{
    saved_sp[8 + 6] = ( uint32_t ) new_entry & ~1u;
}
