# Q1. Comparing ticks that wrap

The kernel counts ticks in a 32-bit unsigned number, `uint32_t`. After
2^32 ticks the count wraps from `0xFFFFFFFF` back to 0, so a later tick
can be a smaller number than an earlier one. In this question, two ticks
are always less than 2^31 ticks apart.

## What you write

In `answer.c`, two functions. Keep their signatures.

**(a)** `int tick_before( uint32_t a, uint32_t b )`

Returns 1 if the tick `a` is earlier than the tick `b`, and 0 otherwise.
If `a` and `b` are the same tick, it returns 0.

**(b)** `uint32_t tick_wait_left( uint32_t now, uint32_t wake )`

Returns how many ticks there are still to wait, from the tick `now`
until the tick `wake`. If `wake` is `now`, or already in the past, it
returns 0. Use `tick_before` for the comparison.

## Examples

These show the meaning of the functions. They are not all the cases.

| call | result |
|---|---|
| `tick_before( 3, 7 )` | 1 |
| `tick_before( 7, 3 )` | 0 |
| `tick_before( 0xFFFFFFFA, 2 )` | 1 |
| `tick_wait_left( 100, 130 )` | 30 |
| `tick_wait_left( 130, 100 )` | 0 |
