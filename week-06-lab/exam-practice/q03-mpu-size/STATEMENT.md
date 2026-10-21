# Q3. Describing an MPU region

A region of the Cortex-M3 memory protection unit has a size that is a
power of two, at least 32 bytes, and a base address that is a multiple
of that size. The `SIZE` field of the region attribute register (`RASR`)
holds `log2(size) - 1`, and sits in bits 5:1 of `RASR`.

For a data region, `RASR` has these fields set:

| Bits | Field | Value |
|---|---|---|
| 28 | XN (no instruction fetch) | 1 |
| 26:24 | AP (access permissions) | `011`, read and write for everyone, or `110`, read only for everyone |
| 18, 17 | S, C | 1, 1 |
| 5:1 | SIZE | `log2(size) - 1` |
| 0 | ENABLE | 1 |

## What you write

In `answer.c`, three functions. Keep their signatures.

**(a)** `uint32_t mpu_size_field( uint32_t bytes )`

Returns the value of the `SIZE` field for a region of `bytes` bytes, that
is `log2(bytes) - 1`. If `bytes` is not a valid region size (not a power
of two, or less than 32), it returns 0.

**(b)** `uint32_t mpu_rasr( uint32_t bytes, int read_only )`

Returns the value of `RASR` for a data region of `bytes` bytes, with
the fields of the table: read and write if `read_only` is 0, read only
otherwise. If `bytes` is not a valid region size, it returns 0. Use
`mpu_size_field`.

**(c)** `int mpu_base_ok( uint32_t base, uint32_t bytes )`

Returns 1 if a region of `bytes` bytes can start at `base`: `bytes` is a
valid region size and `base` is a multiple of it. Otherwise 0. Use
`mpu_size_field` to decide whether the size is valid.

## Examples

These show the meaning of the functions. They are not all the cases.

| call | result |
|---|---|
| `mpu_size_field( 4096 )` | 11 |
| `mpu_size_field( 5000 )` | 0 |
| `mpu_rasr( 4096, 0 )` | 0x13060017 |
| `mpu_base_ok( 0x20001000, 0x8000 )` | 0 |
| `mpu_base_ok( 0x20008000, 0x8000 )` | 1 |
