.intel_syntax noprefix
.text
.globl binary_search

# long binary_search(const long *a, size_t n, long key)
binary_search:
    xor ecx, ecx                # lo = 0
    mov r8, rsi                 # hi = n
.Lloop:
    cmp rcx, r8
    jae .Lnot_found             # lo >= hi: no candidates left
    mov rax, r8
    sub rax, rcx
    shr rax, 1
    add rax, rcx                # mid = lo + (hi - lo) / 2
    mov r9, [rdi + rax*8]
    cmp r9, rdx
    je .Lfound
    jl .Lgo_right
    mov r8, rax                 # a[mid] > key: hi = mid
    jmp .Lloop
.Lgo_right:
    lea rcx, [rax + 1]          # a[mid] < key: lo = mid + 1
    jmp .Lloop
.Lfound:
    ret
.Lnot_found:
    mov rax, -1
    ret

.section .note.GNU-stack,"",@progbits
