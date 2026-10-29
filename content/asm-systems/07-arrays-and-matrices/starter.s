.intel_syntax noprefix
.text
.globl prefix_sums
.globl trace

# void prefix_sums(long *a, size_t n)
# a in rdi, n in rsi. a[i] becomes a[0] + ... + a[i].
prefix_sums:
    ret                 # TODO: replace with the real work

# long trace(const int *m, size_t n)
# m in rdi (n*n ints, row-major), n in rsi. Return the sum of the diagonal in rax.
trace:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
