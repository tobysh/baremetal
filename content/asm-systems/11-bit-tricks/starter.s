.intel_syntax noprefix
.text
.globl bit_count
.globl lowest_set_index
.globl next_pow2

# int bit_count(unsigned long x)           -- popcnt
bit_count:
    mov eax, 0          # TODO: replace with the real work
    ret

# int lowest_set_index(unsigned long x)    -- bsf; -1 when x == 0
lowest_set_index:
    mov eax, 0          # TODO: replace with the real work
    ret

# unsigned long next_pow2(unsigned long n) -- bsr; smallest power of two >= n, at least 1
next_pow2:
    mov eax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
