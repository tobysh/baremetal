.intel_syntax noprefix
.text
.globl popcount64

# int popcount64(uint64_t x)
# x in rdi. Return the number of 1 bits in eax.
popcount64:
    mov eax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
