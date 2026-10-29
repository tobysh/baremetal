.intel_syntax noprefix
.text
.globl divmod_u
.globl floor_div
.globl mod_pos

# uint64_t divmod_u(uint64_t n, uint64_t d, uint64_t *rem)
divmod_u:
    mov rax, 0          # TODO
    ret

# long floor_div(long a, long b)
floor_div:
    mov rax, 0          # TODO
    ret

# long mod_pos(long a, long m)
mod_pos:
    mov rax, -1         # TODO
    ret

.section .note.GNU-stack,"",@progbits
