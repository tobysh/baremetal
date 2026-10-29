.intel_syntax noprefix
.text
.globl binom_rec
.globl ackermann

# unsigned long binom_rec(unsigned long n, unsigned long k)   n in rdi, k in rsi
binom_rec:
    mov rax, 0          # TODO: replace with the real work
    ret

# unsigned long ackermann(unsigned long m, unsigned long n)   m in rdi, n in rsi
ackermann:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
