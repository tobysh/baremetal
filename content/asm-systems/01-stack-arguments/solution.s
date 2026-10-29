.intel_syntax noprefix
.text
.globl weighted8

weighted8:
    mov rax, rdi                # 1*a
    lea rax, [rax + rsi*2]      # + 2*b
    lea rdx, [rdx + rdx*2]
    add rax, rdx                # + 3*c
    lea rax, [rax + rcx*4]      # + 4*d
    lea r8, [r8 + r8*4]
    add rax, r8                 # + 5*e
    imul r9, r9, 6
    add rax, r9                 # + 6*f
    mov r10, [rsp + 8]
    imul r10, r10, 7
    add rax, r10                # + 7*g
    mov r10, [rsp + 16]
    shl r10, 3
    add rax, r10                # + 8*h
    ret

.section .note.GNU-stack,"",@progbits
