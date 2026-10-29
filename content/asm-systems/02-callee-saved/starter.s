.intel_syntax noprefix
.text
.globl count_matching

# size_t count_matching(const long *a, size_t n, long (*pred)(long))
# a in rdi, n in rsi, pred in rdx. Return how many pred(a[i]) are non-zero.
# pred clobbers every caller-saved register. Keep rsp 16-byte aligned at each call.
count_matching:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
