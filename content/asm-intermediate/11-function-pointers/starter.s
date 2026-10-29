.intel_syntax noprefix
.text
.globl map_inplace
.globl fold

# void map_inplace(long *a, size_t n, long (*f)(long))
map_inplace:
    ret                 # TODO

# long fold(const long *a, size_t n, long init, long (*f)(long, long))
fold:
    mov rax, -999       # TODO
    ret

.section .note.GNU-stack,"",@progbits
