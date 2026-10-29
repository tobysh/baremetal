.intel_syntax noprefix
.text
.globl to_decimal

to_decimal:
    push rbp
    mov rbp, rsp
    sub rsp, 32                 # char tmp[32] at [rbp-32, rbp)
    mov r9, rdi                 # remember where buf starts
    mov r8, rbp                 # write cursor: one past the end of tmp
    mov rax, rsi                # the number being divided
    mov rcx, 10
.Ldigit:
    xor edx, edx
    div rcx                     # rax = v / 10, rdx = v % 10
    add dl, 48                  # '0'
    dec r8
    mov [r8], dl
    test rax, rax
    jnz .Ldigit
.Lcopy:
    cmp r8, rbp
    jae .Lend
    mov dl, [r8]
    mov [rdi], dl
    inc r8
    inc rdi
    jmp .Lcopy
.Lend:
    mov byte ptr [rdi], 0
    mov rax, rdi
    sub rax, r9                 # length
    leave
    ret

.section .note.GNU-stack,"",@progbits
