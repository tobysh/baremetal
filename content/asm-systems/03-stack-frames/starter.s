.intel_syntax noprefix
.text
.globl to_decimal

# size_t to_decimal(char *buf, unsigned long v)
# buf in rdi, v in rsi. Write v in decimal plus a NUL; return the digit count in rax.
to_decimal:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
