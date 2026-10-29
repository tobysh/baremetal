.intel_syntax noprefix
.text
.globl count_matching

count_matching:
    push rbx
    push r12
    push r13
    push r14
    push r15                    # five pushes: rsp is 16-byte aligned again
    mov rbx, rdi                # a
    mov r12, rsi                # n
    mov r15, rdx                # pred
    xor r13d, r13d              # i
    xor r14d, r14d              # count
.Lloop:
    cmp r13, r12
    jae .Ldone
    mov rdi, [rbx + r13*8]
    call r15
    test rax, rax
    jz .Lnext
    inc r14
.Lnext:
    inc r13
    jmp .Lloop
.Ldone:
    mov rax, r14
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
