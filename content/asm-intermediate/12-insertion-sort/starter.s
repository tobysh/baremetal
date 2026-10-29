.intel_syntax noprefix
.text
.globl isort
.globl count_inversions

# void isort(long *a, size_t n)
isort:
    ret                 # TODO

# size_t count_inversions(const long *a, size_t n)
count_inversions:
    mov rax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
