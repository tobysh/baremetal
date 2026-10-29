.intel_syntax noprefix
.text
.globl inet_checksum

inet_checksum:
    xor eax, eax
.Lw:
    cmp rsi, 2
    jb .Ltail
    movzx edx, word ptr [rdi]
    add rax, rdx
    add rdi, 2
    sub rsi, 2
    jmp .Lw
.Ltail:
    test rsi, rsi
    jz .Lfold
    movzx edx, byte ptr [rdi]
    add rax, rdx
.Lfold:
    mov rdx, rax
    shr rdx, 16
    jz .Ldone
    movzx eax, ax
    add rax, rdx
    jmp .Lfold
.Ldone:
    not eax
    movzx eax, ax
    ret

.section .note.GNU-stack,"",@progbits
