.intel_syntax noprefix
.text
.globl add_two

# long add_two(long a, long b)
add_two:
    mov rax, rdi
    add rax, rsi
    ret

.section .note.GNU-stack,"",@progbits
