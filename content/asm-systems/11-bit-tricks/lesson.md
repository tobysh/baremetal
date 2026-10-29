---
title: Bit Tricks with popcnt, bsf and bsr
summary: Count set bits, find the lowest or highest one, and round up to a power of two with single instructions instead of loops.
minutes: 25
difficulty: medium
tags: bit-manipulation, instructions
id: asm-systems/bit-tricks
mode: harness
---
In the first track you counted bits with a loop. The CPU can do it in one instruction, and it can also locate the lowest and highest set bit. These are the building blocks of bitmaps, allocators (find a free slot), hash tables and priority queues.

## The instructions

| Instruction | Result |
|---|---|
| `popcnt rax, rdi` | the number of 1 bits in `rdi` |
| `bsf rax, rdi` | index of the **lowest** set bit (bit scan forward) |
| `bsr rax, rdi` | index of the **highest** set bit (bit scan reverse) |

Bits are numbered from 0 at the least significant end. `bsf` of `0b101000` is 3; `bsr` of the same value is 5.

### The empty input trap

For a source of 0, `bsf` and `bsr` have **no answer**. They set the zero flag `ZF = 1` and leave the destination undefined (in practice unchanged). So test the flag right after the instruction:

```asm
bsf rax, rdi
jnz .Lfound             # ZF = 0: rax holds a valid index
mov rax, -1             # source was zero
.Lfound:
ret
```

`popcnt` has no such problem: the count of zero bits set is 0.

## Powers of two

A number `x` is a power of two when it has exactly one bit set. To round `n` up to the next power of two (`5 -> 8`, `8 -> 8`, `1 -> 1`), look at `n - 1`: the highest set bit of `n - 1` is one position below the target bit.

| n | n - 1 | `bsr(n-1)` | answer `1 << (bsr + 1)` |
|---|---|---|---|
| 5 | 4 (100) | 2 | 8 |
| 8 | 7 (111) | 2 | 8 |
| 9 | 8 (1000) | 3 | 16 |
| 1 | 0 | none | 1 |

`n = 0` and `n = 1` both map to 1, and for them `n - 1` is 0 or wraps around, which `bsr` cannot handle sensibly, so special-case `n <= 1` first.

Shifting by a variable amount uses `cl`: `mov ecx, 3` / `shl rax, cl`. It is the only register allowed as the shift count.

## Portability note

`popcnt` came with the Nehalem generation (2008) and is not part of the baseline x86-64 instruction set. Every CPU from the last fifteen years has it, but portable code checks `cpuid` first.
