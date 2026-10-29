.intel_syntax noprefix
.text
.globl add128
.globl mulhi
.globl add_n

add128:
    mov rax, [rdi]
    add rax, [rsi]
    mov rcx, [rdi + 8]
    adc rcx, [rsi + 8]
    mov [rdx], rax
    mov [rdx + 8], rcx
    ret

mulhi:
    mov rax, rdi
    mul rsi
    mov rax, rdx
    ret

add_n:
    xor eax, eax
    test rcx, rcx
    jz .Lan_done
    xor r8d, r8d
    clc
.Lan_loop:
    mov rax, [rdi + r8*8]
    adc rax, [rsi + r8*8]
    mov [rdx + r8*8], rax
    lea r8, [r8 + 1]
    dec rcx
    jnz .Lan_loop
    setc r9b
    movzx eax, r9b
.Lan_done:
    ret

.section .note.GNU-stack,"",@progbits
