`distance`: `subsd xmm0, xmm2` (dx), `subsd xmm1, xmm3` (dy), square each with `mulsd xmm0, xmm0` / `mulsd xmm1, xmm1`, add them, `sqrtsd xmm0, xmm0`, `ret`.
===
`mean`: `pxor xmm0, xmm0` is both the accumulator and the answer for `n = 0`. Loop with an index in `rcx`: `cvtsi2sd xmm1, qword ptr [rdi + rcx*8]` then `addsd xmm0, xmm1`. After the loop, `cvtsi2sd xmm1, rsi` and `divsd xmm0, xmm1`. Test `n` for zero first and skip the division.
===
`floor_to_long`: `cvttsd2si rax, xmm0`, `cvtsi2sd xmm1, rax`, `ucomisd xmm0, xmm1`, `jae .Ldone`, `dec rax`, then `.Ldone: ret`. For exact integers and positive numbers `x >= t` holds and nothing changes.
