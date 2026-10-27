Both functions have the same shape as `sum_array`: zero a total in `rax`, zero an index, and loop while `index < n` using `jae` to leave. Remember that `sum_clamped` already uses `rcx` for `hi`, so keep its index in `r8` instead.
===
For the clamp, load the element into a scratch register such as `r9`. Then `cmp r9, rdx` and skip over `mov r9, rdx` with `jge`; do the same against `rcx` with `jle` and `mov r9, rcx`. These must be **signed** jumps because the values can be negative. Finally `add rax, r9`.
===
For `count_negative`, `cmp qword ptr [rdi + rcx*8], 0` followed by `jge skip` jumps over `inc rax` for elements that are zero or positive. Use label names that differ from the first function's, for example `.Lcn_loop` and `.Lsc_loop`.
