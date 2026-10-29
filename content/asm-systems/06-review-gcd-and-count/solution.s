.intel_syntax noprefix
.text
.globl gcd_rec
.globl count_char

gcd_rec:
    test rsi, rsi
    jnz .Lstep
    mov rax, rdi
    ret
.Lstep:
    mov rax, rdi
    xor edx, edx
    div rsi                     # rdx = a % b
    mov rdi, rsi
    mov rsi, rdx
    jmp gcd_rec                 # tail call

count_char:
    test esi, esi
    jnz .Lstart
    xor eax, eax
    ret
.Lstart:
    push rbx
    push r12
    push r13
    mov rbx, rdi                # cursor
    mov r12d, esi               # c
    xor r13d, r13d              # count
.Lloop:
    mov rdi, rbx
    mov esi, r12d
    call strchr
    test rax, rax
    jz .Ldone
    inc r13
    lea rbx, [rax + 1]
    jmp .Lloop
.Ldone:
    mov rax, r13
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
