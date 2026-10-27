.intel_syntax noprefix
.text
.globl popcount64

# int popcount64(uint64_t x)
popcount64:
    xor eax, eax
.Lloop:
    test rdi, rdi
    jz .Ldone
    lea rcx, [rdi - 1]
    and rdi, rcx
    inc eax
    jmp .Lloop
.Ldone:
    ret

.section .note.GNU-stack,"",@progbits
