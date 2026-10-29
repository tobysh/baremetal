.intel_syntax noprefix
.text
.globl add_checked
.globl mul_checked_u
.globl fact_checked

add_checked:
    mov rax, rdi
    add rax, rsi
    jo .Lac_fail
    mov [rdx], rax
    mov eax, 1
    ret
.Lac_fail:
    xor eax, eax
    ret

mul_checked_u:
    mov r8, rdx
    mov rax, rdi
    mul rsi
    jo .Lmc_fail
    mov [r8], rax
    mov eax, 1
    ret
.Lmc_fail:
    xor eax, eax
    ret

fact_checked:
    mov r8, rsi
    mov ecx, edi
    mov eax, 1
    mov esi, 2
.Lf_loop:
    cmp rsi, rcx
    ja .Lf_ok
    mul rsi
    jo .Lf_fail
    inc rsi
    jmp .Lf_loop
.Lf_ok:
    mov [r8], rax
    mov eax, 1
    ret
.Lf_fail:
    xor eax, eax
    ret

.section .note.GNU-stack,"",@progbits
