.intel_syntax noprefix
.text
.globl divmod_u
.globl floor_div
.globl mod_pos

divmod_u:
    mov r8, rdx
    mov rax, rdi
    xor edx, edx
    div rsi
    mov [r8], rdx
    ret

floor_div:
    mov rax, rdi
    cqo
    idiv rsi
    test rdx, rdx
    jz .Lfd_done
    xor rdx, rsi
    jns .Lfd_done
    dec rax
.Lfd_done:
    ret

mod_pos:
    mov rax, rdi
    cqo
    idiv rsi
    mov rax, rdx
    test rax, rax
    jns .Lmp_done
    add rax, rsi
.Lmp_done:
    ret

.section .note.GNU-stack,"",@progbits
