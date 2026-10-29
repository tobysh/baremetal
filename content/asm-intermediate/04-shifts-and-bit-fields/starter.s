.intel_syntax noprefix
.text
.globl get_bits
.globl rotl32
.globl set_bits

# uint64_t get_bits(uint64_t x, unsigned lo, unsigned len)
get_bits:
    mov rax, 0          # TODO
    ret

# uint32_t rotl32(uint32_t x, unsigned n)
rotl32:
    mov eax, 0          # TODO
    ret

# uint64_t set_bits(uint64_t x, unsigned lo, unsigned len, uint64_t v)
set_bits:
    mov rax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
