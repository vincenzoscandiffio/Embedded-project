/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q2: the reference solution.
 */

#include <stdint.h>

/* The number of bytes of the array: a power of two. */
#define RING_SIZE 8

/* head: index where the next byte is stored; tail: index of the oldest
 * byte. Empty: head == tail. One slot stays unused, so at most
 * RING_SIZE - 1 bytes are in the ring. */
struct ring
{
    uint8_t buf[RING_SIZE];
    unsigned head;
    unsigned tail;
};

/* The index after i, wrapping at the end of the array. RING_SIZE is a
 * power of two, so the wrap is a mask, with no division. */
static unsigned next_index( unsigned i )
{
    return ( i + 1 ) & ( RING_SIZE - 1 );
}

/* (a) Full when the index after head is tail: one slot stays unused, so
 * that full is never confused with empty. */
int ring_put( struct ring *r, uint8_t byte )
{
    if ( next_index( r->head ) == r->tail )
    {
        return 0;
    }
    r->buf[r->head] = byte;
    r->head = next_index( r->head );
    return 1;
}

/* (b) Empty when head equals tail. */
int ring_get( struct ring *r, uint8_t *out )
{
    if ( r->head == r->tail )
    {
        return 0;
    }
    *out = r->buf[r->tail];
    r->tail = next_index( r->tail );
    return 1;
}

/* (c) head - tail, taken modulo the size: the unsigned subtraction wraps
 * and the mask keeps the low bits, which is the distance from tail to
 * head going forward, across the end of the array. */
unsigned ring_count( const struct ring *r )
{
    return ( r->head - r->tail ) & ( RING_SIZE - 1 );
}

/* (d) Store the bytes one at a time with (a), and stop at the first
 * refusal. */
unsigned ring_put_many( struct ring *r, const uint8_t *src, unsigned n )
{
    unsigned stored = 0;
    while ( stored < n && ring_put( r, src[stored] ) )
    {
        stored++;
    }
    return stored;
}
