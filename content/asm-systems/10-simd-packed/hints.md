`add_arrays` (dst `rdi`, a `rsi`, b `rdx`, n `rcx`): vector loop while `rcx >= 4`: `movups xmm0, [rsi]`, `movups xmm1, [rdx]`, `addps xmm0, xmm1`, `movups [rdi], xmm0`; add 16 to the three pointers; `sub rcx, 4`.
===
Tail of `add_arrays`: while `rcx != 0`: `movss xmm0, [rsi]`, `addss xmm0, [rdx]`, `movss [rdi], xmm0`; add 4 to the three pointers; `dec rcx`. Check `cmp rcx, 4` / `jb` for leaving the vector loop; the unsigned compare is correct.
===
`dot_product` (a `rdi`, b `rsi`, n `rdx`): `pxor xmm2, xmm2` (accumulator). Vector loop: load both, `mulps xmm0, xmm1`, `addps xmm2, xmm0`. After it: `haddps xmm2, xmm2` twice. Scalar tail: `movss xmm0, [rdi]`, `mulss xmm0, [rsi]`, `addss xmm2, xmm0`. Finish with `movaps xmm0, xmm2` / `ret`.
