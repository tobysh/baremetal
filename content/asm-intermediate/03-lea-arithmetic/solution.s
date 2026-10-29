.intel_syntax noprefix
.text
.globl times10
.globl nine_plus_five
.globl times85
.globl elem_addr

times10:
    lea rax, [rdi + rdi*4]
    add rax, rax
    ret

nine_plus_five:
    lea rax, [rdi + rdi*8 + 5]
    ret

times85:
    lea rax, [rdi + rdi*4]
    mov rdx, rax
    shl rax, 4
    add rax, rdx
    ret

elem_addr:
    lea rax, [rdi + rsi*8]
    ret

.section .note.GNU-stack,"",@progbits
