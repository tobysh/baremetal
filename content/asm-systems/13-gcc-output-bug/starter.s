.intel_syntax noprefix
.text
.globl count_above

# int count_above(const int *a, int n, int limit)   -- gcc -O0 style, with 3 bugs
#   a     = [rbp - 24]      n = [rbp - 28]      limit = [rbp - 32]
#   count = [rbp - 4]       i = [rbp - 8]
count_above:
    push rbp
    mov rbp, rsp
    mov [rbp - 24], rdi
    mov [rbp - 28], esi
    mov [rbp - 32], edx
    mov dword ptr [rbp - 4], 0          # count = 0
    mov dword ptr [rbp - 8], 0          # i = 0
    jmp .Ltest
.Lbody:
    mov eax, [rbp - 8]
    cdqe
    lea rdx, [rax*4]
    mov rax, [rbp - 24]
    add rax, rdx
    mov eax, [rax]                      # a[i]
    cmp eax, [rbp - 32]                 # a[i] - limit
    jbe .Lskip                          # skip when a[i] <= limit
    add dword ptr [rbp - 4], 1          # count++
.Lskip:
    add dword ptr [rbp - 8], 1          # i++
.Ltest:
    mov eax, [rbp - 8]
    cmp eax, [rbp - 28]                 # i - n
    jle .Lbody                          # continue while i < n
    mov eax, [rbp - 8]                  # return count
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
