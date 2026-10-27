.intel_syntax noprefix
.text
.globl reverse_array

# void reverse_array(long *a, size_t n)
reverse_array:
    test rsi, rsi
    jz .Ldone
    lea rsi, [rdi + rsi*8 - 8]      # rsi = &a[n-1]
.Lloop:
    cmp rdi, rsi
    jae .Ldone
    mov rax, [rdi]
    mov rcx, [rsi]
    mov [rdi], rcx
    mov [rsi], rax
    add rdi, 8
    sub rsi, 8
    jmp .Lloop
.Ldone:
    ret

.section .note.GNU-stack,"",@progbits
