.intel_syntax noprefix
.text
.globl total_score
.globl heaviest

# struct item { int weight; int bonus; long value; };   sizeof == 16
# long total_score(const struct item *p, size_t n)
total_score:
    mov rax, 0          # TODO: replace with the real work
    ret

# long heaviest(const struct item *p, size_t n)   index of first max weight, -1 if n == 0
heaviest:
    mov rax, -2         # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
