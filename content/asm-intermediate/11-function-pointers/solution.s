.intel_syntax noprefix
.text
.globl map_inplace
.globl fold

map_inplace:
    push rbx
    push r12
    push r13
    mov rbx, rdi
    mov r12, rsi
    mov r13, rdx
.Lm_loop:
    test r12, r12
    jz .Lm_done
    mov rdi, [rbx]
    call r13
    mov [rbx], rax
    add rbx, 8
    dec r12
    jmp .Lm_loop
.Lm_done:
    pop r13
    pop r12
    pop rbx
    ret

fold:
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 8
    mov rbx, rdi
    mov r12, rsi
    mov r14, rdx
    mov r13, rcx
.Lf_loop:
    test r12, r12
    jz .Lf_done
    mov rdi, r14
    mov rsi, [rbx]
    call r13
    mov r14, rax
    add rbx, 8
    dec r12
    jmp .Lf_loop
.Lf_done:
    mov rax, r14
    add rsp, 8
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
