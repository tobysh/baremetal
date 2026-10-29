.intel_syntax noprefix
.text
.globl fact_rec
.globl fib_rec

# unsigned long fact_rec(unsigned long n)  -- rdi -> rax, must be recursive
fact_rec:
    mov rax, 0          # TODO: replace with the real work
    ret

# unsigned long fib_rec(unsigned long n)  -- rdi -> rax, must be recursive
fib_rec:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
