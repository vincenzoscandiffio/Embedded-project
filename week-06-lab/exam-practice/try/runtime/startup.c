/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Start-up of the practice questions: the vector table, and the code
 * that runs at reset. A question's program runs alone, privileged, on
 * the main stack, with no kernel: Reset_Handler prepares the memory as
 * week 2's boot code does (copy .data, zero .bss), starts the console
 * and calls main().
 */

#include <stdint.h>
#include "console.h"

/* Symbols defined by common.ld: only their addresses matter. */
extern uint32_t _estack, _etext, _sdata, _edata, _sbss, _ebss;

int main( void );
void Reset_Handler( void );

/* Any exception that is not expected: say so and stop. A question that
 * dereferences a bad pointer ends here, with a message, not in silence. */
static void Default_Handler( void )
{
    kprintf( "fault: the program stopped in an exception handler\n" );
    for ( ;; )
    {
    }
}

/* The vector table: entry 0 is the initial stack pointer, entry 1 the
 * reset vector, the others the system exceptions up to SysTick. */
__attribute__( ( section( ".isr_vector" ), used ) )
void ( *const vector_table[16] )( void ) =
{
    ( void ( * )( void ) ) &_estack,    /* 0: initial stack pointer */
    Reset_Handler,                      /* 1: reset */
    Default_Handler,                    /* 2: NMI */
    Default_Handler,                    /* 3: HardFault */
    Default_Handler,                    /* 4: MemManage */
    Default_Handler,                    /* 5: BusFault */
    Default_Handler,                    /* 6: UsageFault */
    0, 0, 0, 0,                         /* 7-10: reserved */
    Default_Handler,                    /* 11: SVCall */
    Default_Handler,                    /* 12: debug monitor */
    0,                                  /* 13: reserved */
    Default_Handler,                    /* 14: PendSV */
    Default_Handler,                    /* 15: SysTick */
};

void Reset_Handler( void )
{
    /* .data: the initial values are stored in FLASH right after the
     * code (_etext); copy them to RAM, word by word. */
    uint32_t *src = &_etext;
    for ( uint32_t *dst = &_sdata; dst < &_edata; )
    {
        *dst++ = *src++;
    }
    /* .bss: zero it. */
    for ( uint32_t *dst = &_sbss; dst < &_ebss; )
    {
        *dst++ = 0;
    }

    console_init();
    main();
    for ( ;; )
    {
    }
}
