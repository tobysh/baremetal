.intel_syntax noprefix
.text
.globl bit_count
.globl lowest_set_index
.globl next_pow2

bit_count:
    popcnt rax, rdi
    ret

lowest_set_index:
    bsf rax, rdi
    jnz .Lfound
    mov eax, -1
.Lfound:
    ret

next_pow2:
    cmp rdi, 1
    ja .Lbig
    mov eax, 1
    ret
.Lbig:
    lea rax, [rdi - 1]
    bsr rcx, rax
    inc ecx
    mov eax, 1
    shl rax, cl
    ret

.section .note.GNU-stack,"",@progbits
