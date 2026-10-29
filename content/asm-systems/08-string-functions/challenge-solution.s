.intel_syntax noprefix
.text
.globl my_memmove
.globl my_strchr

my_memmove:
    mov rax, rdi
    mov rcx, rdx
    cmp rdi, rsi
    jbe .Lforward           # dst <= src: copying forwards is safe
    lea r8, [rsi + rdx]
    cmp rdi, r8
    jae .Lforward           # dst starts after the source ends: no overlap
    # dst is inside the source: copy backwards
    test rcx, rcx
    jz .Ldone
    lea rsi, [rsi + rdx - 1]
    lea rdi, [rdi + rdx - 1]
    std
    rep movsb
    cld
    ret
.Lforward:
    rep movsb
.Ldone:
    ret

my_strchr:
.Lscan:
    movzx eax, byte ptr [rdi]
    cmp al, sil
    je .Lfound
    test eax, eax
    jz .Lnone
    inc rdi
    jmp .Lscan
.Lfound:
    mov rax, rdi
    ret
.Lnone:
    xor eax, eax
    ret

.section .note.GNU-stack,"",@progbits
