.intel_syntax noprefix
.text
.globl sum_array

# long sum_array(const long *a, size_t n)
# a in rdi, n in rsi. Return the sum in rax (0 for an empty array).
sum_array:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
