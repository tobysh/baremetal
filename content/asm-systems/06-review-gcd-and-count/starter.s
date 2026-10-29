.intel_syntax noprefix
.text
.globl gcd_rec
.globl count_char

# unsigned long gcd_rec(unsigned long a, unsigned long b)   -- recursive
gcd_rec:
    mov rax, 0          # TODO: replace with the real work
    ret

# size_t count_char(const char *s, int c)   -- use strchr; c == 0 returns 0
count_char:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
