.intel_syntax noprefix
.text
.globl sum_i8
.globl sum_u8
.globl sum_i32

# long sum_i8(const int8_t *p, size_t n)
sum_i8:
    mov rax, 0          # TODO
    ret

# unsigned long sum_u8(const uint8_t *p, size_t n)
sum_u8:
    mov rax, 0          # TODO
    ret

# long sum_i32(const int32_t *p, size_t n)
sum_i32:
    mov rax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
