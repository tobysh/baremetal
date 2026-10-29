.intel_syntax noprefix
.text
.globl u64_to_dec
.globl parse_dec

# size_t u64_to_dec(uint64_t v, char *buf)
u64_to_dec:
    mov rax, 0          # TODO
    ret

# long parse_dec(const char *s)
parse_dec:
    mov rax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
