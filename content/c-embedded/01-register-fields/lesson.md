---
title: Register Fields
summary: Change one field of a memory-mapped register without disturbing its neighbours, and know exactly how often the register is touched.
minutes: 20
difficulty: easy
tags: embedded, registers, bit-fields
id: c-embedded/register-fields
mode: harness
---
A **memory-mapped register** is a word at a fixed address that is really a piece of hardware. A control register packs several settings, called *fields*, into one 32-bit word:

```text
bit   31 ........ 12 | 11 ... 4 | 3 2 | 1 0
      reserved          DIV       MODE   -  ENABLE is bit 0
```

To change MODE alone you must **read-modify-write** (RMW): read the word, clear the MODE bits, put the new value in, write the word back. Writing only `mode << 1` would silently switch off ENABLE and zero DIV.

## The bus in this track

Real code often dereferences a `volatile uint32_t *`. The `volatile` keyword tells the compiler that every access matters: it may not cache a read in a register, merge two reads, or drop a write whose value is never read back by the program. In this track the hardware is simulated, so every driver talks through a small interface instead:

```c
typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);            // register read, may have side effects
    void     (*write)(struct bus *b, uint32_t off, uint32_t v); // register write
    uint32_t (*now)(struct bus *b);                           // fake tick counter
} bus_t;
```

A call through a function pointer cannot be optimised away, so the simulator sees every access. **Limitation:** this teaches the access *contract*, not `volatile` itself. Whether a compiler keeps a `volatile` access is not something these tests can check.

## Masks without undefined behaviour

A field is described by its `shift` and `width`. Its mask is `((1u << width) - 1) << shift`, except for `width == 32` where `1u << 32` is undefined behaviour, so that case needs `0xFFFFFFFF`.

## Your turn

Implement `field_get` and `field_set`. Both must touch the register the minimum number of times: a read-modify-write is **one read and one write**, and a get is one read and no write. Extra reads matter on real devices whose registers change when read.
`field_get(b, off, shift, width)` returns the field value, shifted down to bit 0.
`field_set(b, off, shift, width, val)` changes only that field.

- `field_get` performs exactly one read and no writes, and the result contains no bits outside the field.
- `field_set` performs exactly one read and exactly one write of register `off`. Bits outside the field keep their value from that read.
- If `val` is wider than the field, keep only its low `width` bits. Other fields must not be affected.
- `width` is between 1 and 32 and `shift + width <= 32`. A 32-bit field at shift 0 must work.

Many offsets, field sizes and old register contents are tried.
