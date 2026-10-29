.intel_syntax noprefix
.text
.globl isort
.globl count_inversions

isort:
    mov r8d, 1
.Ls_outer:
    cmp r8, rsi
    jae .Ls_done
    mov rax, [rdi + r8*8]
    mov rcx, r8
.Ls_inner:
    test rcx, rcx
    jz .Ls_place
    mov rdx, [rdi + rcx*8 - 8]
    cmp rdx, rax
    jle .Ls_place
    mov [rdi + rcx*8], rdx
    dec rcx
    jmp .Ls_inner
.Ls_place:
    mov [rdi + rcx*8], rax
    inc r8
    jmp .Ls_outer
.Ls_done:
    ret

count_inversions:
    xor eax, eax
    xor r8d, r8d
.Lc_i:
    cmp r8, rsi
    jae .Lc_done
    mov rdx, [rdi + r8*8]
    lea r9, [r8 + 1]
.Lc_j:
    cmp r9, rsi
    jae .Lc_next
    xor ecx, ecx
    cmp rdx, [rdi + r9*8]
    setg cl
    add rax, rcx
    inc r9
    jmp .Lc_j
.Lc_next:
    inc r8
    jmp .Lc_i
.Lc_done:
    ret

.section .note.GNU-stack,"",@progbits
