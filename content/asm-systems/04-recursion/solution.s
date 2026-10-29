.intel_syntax noprefix
.text
.globl fact_rec
.globl fib_rec

fact_rec:
    cmp rdi, 1
    ja .Lrecurse
    mov eax, 1
    ret
.Lrecurse:
    push rdi
    dec rdi
    call fact_rec
    pop rdi
    imul rax, rdi
    ret

fib_rec:
    cmp rdi, 2
    jb .Lbase
    sub rsp, 24
    mov [rsp], rdi
    dec rdi
    call fib_rec
    mov [rsp + 8], rax
    mov rdi, [rsp]
    sub rdi, 2
    call fib_rec
    add rax, [rsp + 8]
    add rsp, 24
    ret
.Lbase:
    mov rax, rdi
    ret

.section .note.GNU-stack,"",@progbits
