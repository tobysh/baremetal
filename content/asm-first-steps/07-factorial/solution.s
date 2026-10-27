.intel_syntax noprefix
.text
.globl factorial

# unsigned long factorial(unsigned long n)
factorial:
    cmp rdi, 1
    jbe .Lbase
    push rbx                # save rbx; also realigns rsp to 16
    mov rbx, rdi            # keep n across the call
    lea rdi, [rdi - 1]
    call factorial
    imul rax, rbx
    pop rbx
    ret
.Lbase:
    mov eax, 1
    ret

.section .note.GNU-stack,"",@progbits
