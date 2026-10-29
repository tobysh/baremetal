.intel_syntax noprefix
.text
.globl my_memmove
.globl my_strchr

# void *my_memmove(void *dst, const void *src, size_t n)   returns dst; regions may overlap
my_memmove:
    mov rax, 0          # TODO: replace with the real work
    ret

# char *my_strchr(const char *s, int c)   NULL when absent; c == 0 finds the terminator
my_strchr:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
