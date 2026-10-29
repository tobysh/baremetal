---
title: Packed SIMD with SSE
summary: One instruction, four floats. Use movups, addps and mulps to process arrays four elements at a time, then finish the leftovers with the scalar instructions.
minutes: 40
difficulty: hard
tags: simd, sse, floating-point, loops
id: asm-systems/simd-packed
mode: harness
---
An `xmm` register is 128 bits wide: four `float`s or two `double`s. **Packed** instructions apply the same operation to every lane at once. The suffix tells the lane type: `ps` is packed single (4 floats), `pd` packed double (2 doubles). Compare:

| Scalar | Packed | Does |
|---|---|---|
| `addss xmm0, xmm1` | `addps xmm0, xmm1` | add 1 lane / 4 lanes |
| `mulss` | `mulps` | multiply |
| `movss xmm0, [rdi]` | `movups xmm0, [rdi]` | load 4 bytes / 16 bytes |

```asm
movups xmm0, [rsi]        # a[i..i+3]
movups xmm1, [rdx]        # b[i..i+3]
addps  xmm0, xmm1         # four additions
movups [rdi], xmm0        # store four results
```

## Aligned or unaligned?

`movaps` requires the address to be a multiple of 16 and faults otherwise. `movups` accepts any address and on modern CPUs costs the same when the data happens to be aligned. When you do not control the caller's pointers, use `movups`.

## The vector loop and the tail

Four lanes at a time means the loop count is `n / 4`, and `n % 4` elements remain. The standard structure:

```text
while (n >= 4) { process 4 floats; advance by 16 bytes; n -= 4; }
while (n >  0) { process 1 float;  advance by  4 bytes; n -= 1; }
```

Never read the elements that do not exist. The hidden test places arrays flush against an inaccessible page, so a vector load of "the last 4 floats" when only 3 remain crashes.

## Horizontal sums

A dot product multiplies lane by lane and then needs to add the four lanes together. Accumulate four partial sums in one register inside the loop (`mulps` then `addps`), and reduce once at the end with SSE3's `haddps`, which adds neighbouring pairs:

```asm
haddps xmm2, xmm2      # (a+b, c+d, a+b, c+d)
haddps xmm2, xmm2      # (a+b+c+d, ...)  -> lane 0 holds the total
```

Then add the scalar tail with `mulss` / `addss` into lane 0. A `float` result is returned in the low lane of `xmm0`.

## Why bother?

A packed loop processes four numbers for roughly the cost of one. Compilers do this automatically ("auto-vectorisation"), and this is the hand-written core underneath image filters, audio mixers and physics engines. Floating-point addition is not associative, so summing in a different order can change the last bits; the hidden test uses small whole numbers, which are exact in any order.
