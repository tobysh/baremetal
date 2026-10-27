.intel_syntax noprefix
.text
.globl sum_clamped
.globl count_negative

# long sum_clamped(const long *a, size_t n, long lo, long hi)
# a in rdi, n in rsi, lo in rdx, hi in rcx. Return the sum in rax.
sum_clamped:
    mov rax, 0          # TODO: replace with the real work
    ret

# size_t count_negative(const long *a, size_t n)
# a in rdi, n in rsi. Return the count in rax.
count_negative:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
