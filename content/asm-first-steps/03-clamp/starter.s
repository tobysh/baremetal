.intel_syntax noprefix
.text
.globl clamp

# long clamp(long x, long lo, long hi)
# x in rdi, lo in rsi, hi in rdx. Return the clamped value in rax.
clamp:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
