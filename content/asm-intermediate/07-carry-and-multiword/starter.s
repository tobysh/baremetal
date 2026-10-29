.intel_syntax noprefix
.text
.globl add128
.globl mulhi
.globl add_n

# void add128(const uint64_t *a, const uint64_t *b, uint64_t *out)
add128:
    ret                 # TODO

# uint64_t mulhi(uint64_t a, uint64_t b)
mulhi:
    mov rax, 0          # TODO
    ret

# unsigned add_n(const uint64_t *a, const uint64_t *b, uint64_t *out, size_t n)
add_n:
    mov eax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
