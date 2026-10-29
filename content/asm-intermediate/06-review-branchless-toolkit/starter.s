.intel_syntax noprefix
.text
.globl clamp_byte
.globl count_in_range
.globl nibble_at

# int clamp_byte(long x)
clamp_byte:
    mov eax, -1         # TODO
    ret

# size_t count_in_range(const int8_t *p, size_t n, int lo, int hi)
count_in_range:
    mov rax, -1         # TODO
    ret

# unsigned nibble_at(uint64_t x, unsigned idx)
nibble_at:
    mov eax, 16         # TODO
    ret

.section .note.GNU-stack,"",@progbits
