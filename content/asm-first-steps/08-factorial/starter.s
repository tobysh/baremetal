.intel_syntax noprefix
.text
.globl factorial

# unsigned long factorial(unsigned long n)
# n in rdi. Return n! in rax. Make it recursive.
# Remember: push/pop any callee-saved register you use, and keep rsp
# 16-byte aligned when you call.
factorial:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
