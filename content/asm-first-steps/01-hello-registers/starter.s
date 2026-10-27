.intel_syntax noprefix
.text
.globl add_two

# long add_two(long a, long b)
# a arrives in rdi, b in rsi. Return a + b in rax.
add_two:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
