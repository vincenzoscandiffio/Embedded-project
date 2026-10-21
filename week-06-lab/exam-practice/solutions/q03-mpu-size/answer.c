/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q3: the reference solution.
 */

#include <stdint.h>

/* The fields of RASR for a data region (week 4, "Decoding RASR"). */
#define RASR_XN         0x10000000u     /* bit 28: no instruction fetch */
#define RASR_AP_RW      0x03000000u     /* bits 26:24 = 011: read/write for all */
#define RASR_AP_RO      0x06000000u     /* bits 26:24 = 110: read only for all */
#define RASR_S_C        0x00060000u     /* bits 18, 17: shareable, cacheable */
#define RASR_ENABLE     0x00000001u     /* bit 0 */

/* (a) A power of two has exactly one bit set, so x & (x - 1) is 0 for it
 * (and for 0, which the "less than 32" test rejects). __builtin_ctz
 * gives log2 of a power of two; the SIZE field is that minus 1. */
uint32_t mpu_size_field( uint32_t bytes )
{
    if ( bytes < 32 || ( bytes & ( bytes - 1 ) ) != 0 )
    {
        return 0;
    }
    return ( uint32_t ) __builtin_ctz( bytes ) - 1;
}

/* (b) SIZE sits in bits 5:1: shifted left by one. A size that (a) rejects
 * gives 0, which is also what this returns for it. */
uint32_t mpu_rasr( uint32_t bytes, int read_only )
{
    uint32_t size = mpu_size_field( bytes );
    if ( size == 0 )
    {
        return 0;
    }
    return RASR_XN | ( read_only ? RASR_AP_RO : RASR_AP_RW ) | RASR_S_C
           | ( size << 1 ) | RASR_ENABLE;
}

/* (c) A multiple of a power of two has its low log2 bits at 0: base &
 * (bytes - 1) is 0. */
int mpu_base_ok( uint32_t base, uint32_t bytes )
{
    if ( mpu_size_field( bytes ) == 0 )
    {
        return 0;
    }
    return ( base & ( bytes - 1 ) ) == 0;
}
