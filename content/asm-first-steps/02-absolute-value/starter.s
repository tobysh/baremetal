.intel_syntax noprefix
.text
.globl abs_val

# long abs_val(long x)
# x arrives in rdi. Return |x| in rax.
# Hint: neg sets the flags, cmovs copies only if the sign flag is set.
abs_val:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
