.intel_syntax noprefix
.text
.globl fill_bytes
.globl move_bytes
.globl str_length

fill_bytes:
    mov r8, rdi
    mov eax, esi
    mov rcx, rdx
    rep stosb
    mov rax, r8
    ret

move_bytes:
    mov rax, rdi
    mov rcx, rdx
    cmp rdi, rsi
    jbe .Lm_fwd
    lea r8, [rsi + rdx]
    cmp rdi, r8
    jae .Lm_fwd
    std
    lea rdi, [rdi + rcx - 1]
    lea rsi, [rsi + rcx - 1]
    rep movsb
    cld
    ret
.Lm_fwd:
    rep movsb
    ret

str_length:
    xor eax, eax
    mov rcx, -1
    repne scasb
    not rcx
    dec rcx
    mov rax, rcx
    ret

.section .note.GNU-stack,"",@progbits
