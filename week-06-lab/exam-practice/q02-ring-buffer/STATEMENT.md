# Q2. A ring buffer

A ring buffer is an array used as a queue. The producer stores bytes at
index `head`, the consumer takes them at index `tail`, and both indices
wrap to 0 at the end of the array. Here the array has `RING_SIZE` = 8
bytes, so the indices are 0 to 7. The rules are:

- the ring is **empty** when `head == tail`;
- the ring is **full** when the index after `head` is `tail`; one slot is
  always left unused, so a full ring holds 7 bytes;
- `head` changes only when a byte is stored, `tail` only when a byte is
  taken.

The structure, already defined in `answer.c`:

```c
struct ring { uint8_t buf[RING_SIZE]; unsigned head; unsigned tail; };
```

Every function receives a ring that is in a valid state (`head` and
`tail` between 0 and 7), possibly with `head` and `tail` anywhere.

## What you write

In `answer.c`, four functions. Keep their signatures.

**(a)** `int ring_put( struct ring *r, uint8_t byte )`

Stores `byte` at `head` and moves `head` on, and returns 1. If the ring
is full, it changes nothing and returns 0.

**(b)** `int ring_get( struct ring *r, uint8_t *out )`

Takes the oldest byte, writes it to `*out`, moves `tail` on, and returns
1. If the ring is empty, it changes nothing and returns 0.

**(c)** `unsigned ring_count( const struct ring *r )`

Returns how many bytes are in the ring (0 to 7), for any `head` and
`tail`.

**(d)** `unsigned ring_put_many( struct ring *r, const uint8_t *src, unsigned n )`

Stores the bytes `src[0]`, `src[1]`, ... in this order, until all `n`
are stored or the ring is full, and returns how many it stored.

## Examples

These show the meaning of the functions. They are not all the cases. In
the table, `(struct ring){ {0}, 6, 2 }` is a ring with `head` 6, `tail` 2
and all bytes 0.

| call | result |
|---|---|
| `ring_count( &(struct ring){ {0}, 6, 2 } )` | 4 |
| `ring_count( &(struct ring){ {0}, 1, 6 } )` | 3 |
| `ring_put( &(struct ring){ {0}, 7, 0 }, 9 )` | 0 |
| `ring_put( &(struct ring){ {0}, 2, 5 }, 9 )` | 1 |
| `ring_get( &(struct ring){ {0}, 3, 3 }, &(uint8_t){ 0 } )` | 0 |
