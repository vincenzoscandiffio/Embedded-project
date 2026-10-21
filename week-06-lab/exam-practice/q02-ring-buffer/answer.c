/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q2: your answer. See STATEMENT.md.
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

/* (a) */
int ring_put( struct ring *r, uint8_t byte )
{
    ( void ) r;
    ( void ) byte;
    return 0;                   /* replace with your code */
}

/* (b) */
int ring_get( struct ring *r, uint8_t *out )
{
    ( void ) r;
    ( void ) out;
    return 0;                   /* replace with your code */
}

/* (c) */
unsigned ring_count( const struct ring *r )
{
    ( void ) r;
    return 0;                   /* replace with your code */
}

/* (d) */
unsigned ring_put_many( struct ring *r, const uint8_t *src, unsigned n )
{
    ( void ) r;
    ( void ) src;
    ( void ) n;
    return 0;                   /* replace with your code */
}
