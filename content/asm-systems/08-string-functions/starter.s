.intel_syntax noprefix
.text
.globl my_strlen
.globl my_memcpy
.globl my_strcmp

# size_t my_strlen(const char *s)
my_strlen:
    mov rax, 0          # TODO: replace with the real work
    ret

# void *my_memcpy(void *dst, const void *src, size_t n)   returns dst
my_memcpy:
    mov rax, 0          # TODO: replace with the real work
    ret

# int my_strcmp(const char *a, const char *b)   unsigned bytes; sign of the result matters
my_strcmp:
    mov eax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
