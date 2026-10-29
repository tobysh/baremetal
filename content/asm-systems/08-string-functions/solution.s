.intel_syntax noprefix
.text
.globl my_strlen
.globl my_memcpy
.globl my_strcmp

my_strlen:
    xor eax, eax
.Llen:
    cmp byte ptr [rdi + rax], 0
    je .Llen_done
    inc rax
    jmp .Llen
.Llen_done:
    ret

my_memcpy:
    mov rax, rdi
    mov rcx, rdx
    rep movsb
    ret

my_strcmp:
.Lcmp:
    movzx eax, byte ptr [rdi]
    movzx ecx, byte ptr [rsi]
    cmp eax, ecx
    jne .Ldiff
    test eax, eax
    jz .Lequal
    inc rdi
    inc rsi
    jmp .Lcmp
.Ldiff:
    sub eax, ecx
.Lequal:
    ret

.section .note.GNU-stack,"",@progbits
