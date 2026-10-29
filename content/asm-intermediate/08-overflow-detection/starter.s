.intel_syntax noprefix
.text
.globl add_checked
.globl mul_checked_u
.globl fact_checked

# int add_checked(long a, long b, long *out)
add_checked:
    mov eax, 0          # TODO
    ret

# int mul_checked_u(uint64_t a, uint64_t b, uint64_t *out)
mul_checked_u:
    mov eax, 0          # TODO
    ret

# int fact_checked(unsigned n, uint64_t *out)
fact_checked:
    mov eax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
