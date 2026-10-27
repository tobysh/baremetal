.intel_syntax noprefix
.text
.globl clamp

# long clamp(long x, long lo, long hi)
clamp:
    mov rax, rdi
    cmp rax, rsi
    jge .Lcheck_high
    mov rax, rsi
    ret
.Lcheck_high:
    cmp rax, rdx
    jle .Ldone
    mov rax, rdx
.Ldone:
    ret

.section .note.GNU-stack,"",@progbits
