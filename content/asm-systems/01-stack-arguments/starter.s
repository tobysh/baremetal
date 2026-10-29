.intel_syntax noprefix
.text
.globl weighted8

# long weighted8(long a, long b, long c, long d, long e, long f, long g, long h)
# a..f in rdi, rsi, rdx, rcx, r8, r9; g at [rsp+8], h at [rsp+16].
# Return 1*a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h in rax.
weighted8:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
