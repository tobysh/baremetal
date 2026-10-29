.intel_syntax noprefix
.text
.globl max3
.globl sign_of
.globl sat_sub

max3:
    mov rax, rdi
    cmp rax, rsi
    cmovl rax, rsi
    cmp rax, rdx
    cmovl rax, rdx
    ret

sign_of:
    xor eax, eax
    xor ecx, ecx
    test rdi, rdi
    setg al
    setl cl
    sub eax, ecx
    ret

sat_sub:
    xor ecx, ecx
    mov rax, rdi
    sub rax, rsi
    cmovb rax, rcx
    ret

.section .note.GNU-stack,"",@progbits
