.intel_syntax noprefix
.text
.globl abs_val

# long abs_val(long x)
abs_val:
    mov rax, rdi
    neg rax
    cmovs rax, rdi
    ret

.section .note.GNU-stack,"",@progbits
