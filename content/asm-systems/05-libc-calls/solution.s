.intel_syntax noprefix
.text
.globl format_pair
.globl joined_length

.section .rodata
.Lfmt: .asciz "%ld,%ld"

.text
format_pair:
    mov r8, rcx                 # b: second vararg
    mov rcx, rdx                # a: first vararg
    lea rdx, [rip + .Lfmt]
    sub rsp, 8                  # align the stack for the call
    xor eax, eax                # no vector registers used
    call snprintf
    add rsp, 8
    ret

joined_length:
    sub rsp, 24                 # alignment + two slots
    mov [rsp], rsi              # keep b
    call strlen                 # a is already in rdi
    mov [rsp + 8], rax
    mov rdi, [rsp]
    call strlen
    add rax, [rsp + 8]
    add rsp, 24
    ret

.section .note.GNU-stack,"",@progbits
