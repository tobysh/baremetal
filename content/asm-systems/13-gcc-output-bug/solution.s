.intel_syntax noprefix
.text
.globl count_above

count_above:
    push rbp
    mov rbp, rsp
    mov [rbp - 24], rdi
    mov [rbp - 28], esi
    mov [rbp - 32], edx
    mov dword ptr [rbp - 4], 0
    mov dword ptr [rbp - 8], 0
    jmp .Ltest
.Lbody:
    mov eax, [rbp - 8]
    cdqe
    lea rdx, [rax*4]
    mov rax, [rbp - 24]
    add rax, rdx
    mov eax, [rax]
    cmp eax, [rbp - 32]
    jle .Lskip                          # signed: skip when a[i] <= limit
    add dword ptr [rbp - 4], 1
.Lskip:
    add dword ptr [rbp - 8], 1
.Ltest:
    mov eax, [rbp - 8]
    cmp eax, [rbp - 28]
    jl .Lbody                           # strictly less
    mov eax, [rbp - 4]                  # count
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
