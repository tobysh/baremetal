.intel_syntax noprefix
.text
.globl max_array

# long max_array(const long *a, size_t n)
# a in rdi, n in rsi (n >= 1). Return the largest value in rax.
max_array:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
