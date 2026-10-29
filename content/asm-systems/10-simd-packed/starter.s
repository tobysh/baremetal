.intel_syntax noprefix
.text
.globl add_arrays
.globl dot_product

# void add_arrays(float *dst, const float *a, const float *b, size_t n)
# dst rdi, a rsi, b rdx, n rcx. dst[i] = a[i] + b[i]. Four lanes at a time, then a scalar tail.
add_arrays:
    ret                 # TODO: replace with the real work

# float dot_product(const float *a, const float *b, size_t n)
# a rdi, b rsi, n rdx. Return the sum of a[i]*b[i] in xmm0.
dot_product:
    pxor xmm0, xmm0     # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
