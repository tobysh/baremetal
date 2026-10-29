.intel_syntax noprefix
.text
.globl apply_op
.globl op_name

# long apply_op(long op, long a, long b)
# op in rdi, a in rsi, b in rdx. 0 add, 1 sub, 2 mul, 3 and, 4 or, 5 xor, 6 shl (b & 63), 7 signed max.
# Anything else returns -1. Use a jump table.
apply_op:
    mov rax, -1         # TODO: replace with the real work
    ret

# const char *op_name(long op)   "add" "sub" "mul" "and" "or" "xor" "shl" "max", or "?"
op_name:
    xor eax, eax        # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
