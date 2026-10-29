---
title: Shifts, Rotates and Bit Fields
summary: Variable shifts take their count in cl. Use them to extract and replace bit fields and to rotate words, the way hashes, flags and protocols pack data.
minutes: 30
difficulty: medium
tags: bit-manipulation, instructions
id: asm-intermediate/shifts-and-bit-fields
mode: harness
---
Packed data is everywhere: a page-table entry, an IP header, a hash state. Reading and writing a field means shifting and masking.

## Shift counts live in `cl`

`shl`, `shr`, `sar`, `rol` and `ror` accept either an immediate (`shl rax, 3`) or the register `cl` (`shl rax, cl`). **No other register works.** If `cl` is already in use, move things around first. In particular `rcx` is the 4th argument register, so a function with four arguments must save `rcx` somewhere before it can use `cl` as a count.

| Instruction | Effect |
|---|---|
| `shl` | shift left, fill with 0 |
| `shr` | logical shift right, fill with 0 (unsigned `>>`) |
| `sar` | arithmetic shift right, copies the sign (signed `>>`) |
| `rol`/`ror` | rotate: bits that fall off one end re-enter at the other |

## Masks

A mask of `len` ones is `(1 << len) - 1`:

```asm
mov ecx, edx            # len
mov eax, 1
shl rax, cl             # 1 << len
dec rax                 # len ones
```

Extract the field with `(x >> lo) & mask`. Replace it by clearing `mask << lo` in `x` and OR-ing in `(v & mask) << lo`.

`rol eax, cl` on a 32-bit register rotates 32 bits; `rol rax, cl` rotates 64.
