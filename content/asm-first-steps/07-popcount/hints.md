Keep the count in `eax`, starting at zero with `xor eax, eax`. Loop until `rdi` is zero (`test rdi, rdi` then `jz`).
===
`x & (x - 1)` removes the lowest set bit. Compute `x - 1` into a scratch register with `lea rcx, [rdi - 1]`, then `and rdi, rcx`.
===
Each trip through the loop removes exactly one set bit, so add one to `eax` per trip with `inc eax`. All-ones input (64 bits) simply loops 64 times.
