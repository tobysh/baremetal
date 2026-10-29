.intel_syntax noprefix
.text
.globl distance
.globl mean
.globl floor_to_long

# double distance(double x1, double y1, double x2, double y2)   xmm0..xmm3 -> xmm0
distance:
    pxor xmm0, xmm0     # TODO: replace with the real work
    ret

# double mean(const long *a, size_t n)   rdi, rsi -> xmm0 (0.0 when n == 0)
mean:
    pxor xmm0, xmm0     # TODO: replace with the real work
    ret

# long floor_to_long(double x)   xmm0 -> rax
floor_to_long:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
