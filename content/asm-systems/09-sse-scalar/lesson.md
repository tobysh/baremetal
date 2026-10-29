---
title: Floating Point with SSE
summary: Doubles live in xmm registers, not rax. Use addsd, mulsd, sqrtsd and the conversion instructions to compute a distance, an average and a floor.
minutes: 30
difficulty: medium
tags: floating-point, sse, calling-convention
id: asm-systems/sse-scalar
mode: harness
---
x86-64 does floating point in the **SSE** unit: sixteen 128-bit registers `xmm0`-`xmm15`. The calling convention uses them for `float` and `double`:

- The first eight floating-point arguments arrive in `xmm0`-`xmm7`, independently of the integer arguments in `rdi`, `rsi`, ...
- A `double` result is returned in `xmm0`.
- **All** `xmm` registers are caller-saved: no need to preserve any.

So `double distance(double x1, double y1, double x2, double y2)` finds its four arguments in `xmm0`, `xmm1`, `xmm2`, `xmm3`. And `double mean(const long *a, size_t n)` finds the pointer in `rdi`, `n` in `rsi`, and returns in `xmm0`.

## Scalar instructions

The suffix says what the instruction operates on: `sd` is scalar double (one 64-bit value in the low half of the register), `ss` is scalar single (a 32-bit `float`).

| C | Instruction |
|---|---|
| `a + b` | `addsd xmm0, xmm1` |
| `a - b` | `subsd xmm0, xmm1` |
| `a * b` | `mulsd xmm0, xmm1` |
| `a / b` | `divsd xmm0, xmm1` |
| `sqrt(a)` | `sqrtsd xmm0, xmm0` |
| copy | `movsd xmm0, xmm1`, or `movaps` (faster to decode) |
| zero | `pxor xmm0, xmm0` |

The first operand is both input and output, as with integer `add`. Memory operands work for the second operand: `addsd xmm0, [rdi]`.

## Integer to float and back

```asm
cvtsi2sd xmm1, rax                       # (double)(long)rax
cvtsi2sd xmm1, qword ptr [rdi + rcx*8]   # straight from memory (needs the size)
cvttsd2si rax, xmm0                      # (long)x, truncating toward zero
cvtsd2si rax, xmm0                       # rounds using the current mode (nearest even)
```

The extra `t` in `cvttsd2si` means "truncate", which is what a C cast does.

## Comparing: `ucomisd`

`ucomisd xmm0, xmm1` compares two doubles and sets flags like an *unsigned* integer compare: `ja` (above), `jae`, `jb` (below), `je`. Use the unsigned jumps, not `jg` or `jl`. If either operand is NaN the flags say "unordered" (`jp` is taken).

## Floor

C's cast truncates toward zero, so `(long)-2.5` is `-2`, but `floor(-2.5)` is `-3`. A floor is: truncate, convert back, and if the original was *below* the truncated value (only possible for negative non-integers), subtract one:

```asm
cvttsd2si rax, xmm0          # t = (long)x
cvtsi2sd  xmm1, rax          # (double)t
ucomisd   xmm0, xmm1         # compare x with t
jae       .Ldone             # x >= t: fine
dec       rax                # x < t: step down
```

## Constants

There is no "load immediate double". Either build it from an integer (`mov eax, 2` then `cvtsi2sd xmm1, rax`), or put it in `.rodata` as `.double 0.5` and load with `movsd xmm1, [rip + .Lhalf]`.
