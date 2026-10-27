.intel_syntax noprefix
.text
.globl sum_array

# long sum_array(const long *a, size_t n)
sum_array:
    xor eax, eax            # total = 0
    xor ecx, ecx            # i = 0
.Lloop:
    cmp rcx, rsi
    jae .Ldone
    add rax, [rdi + rcx*8]
    inc rcx
    jmp .Lloop
.Ldone:
    ret

.section .note.GNU-stack,"",@progbits
