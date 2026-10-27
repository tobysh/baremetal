.intel_syntax noprefix
.text
.globl binary_search

# long binary_search(const long *a, size_t n, long key)
# a in rdi, n in rsi, key in rdx. a is sorted ascending.
# Return an index of key in rax, or -1 if absent.
binary_search:
    mov rax, -2         # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
