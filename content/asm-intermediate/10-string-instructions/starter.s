.intel_syntax noprefix
.text
.globl fill_bytes
.globl move_bytes
.globl str_length

# void *fill_bytes(void *dst, int c, size_t n)
fill_bytes:
    mov rax, 0          # TODO
    ret

# void *move_bytes(void *dst, const void *src, size_t n)
move_bytes:
    mov rax, 0          # TODO
    ret

# size_t str_length(const char *s)
str_length:
    mov rax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
