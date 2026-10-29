.intel_syntax noprefix
.text
.globl add_arrays
.globl dot_product

add_arrays:
.Ladd_vec:
    cmp rcx, 4
    jb .Ladd_tail
    movups xmm0, [rsi]
    movups xmm1, [rdx]
    addps xmm0, xmm1
    movups [rdi], xmm0
    add rdi, 16
    add rsi, 16
    add rdx, 16
    sub rcx, 4
    jmp .Ladd_vec
.Ladd_tail:
    test rcx, rcx
    jz .Ladd_done
    movss xmm0, [rsi]
    addss xmm0, [rdx]
    movss [rdi], xmm0
    add rdi, 4
    add rsi, 4
    add rdx, 4
    dec rcx
    jmp .Ladd_tail
.Ladd_done:
    ret

dot_product:
    pxor xmm2, xmm2
.Ldot_vec:
    cmp rdx, 4
    jb .Ldot_reduce
    movups xmm0, [rdi]
    movups xmm1, [rsi]
    mulps xmm0, xmm1
    addps xmm2, xmm0
    add rdi, 16
    add rsi, 16
    sub rdx, 4
    jmp .Ldot_vec
.Ldot_reduce:
    haddps xmm2, xmm2
    haddps xmm2, xmm2
.Ldot_tail:
    test rdx, rdx
    jz .Ldot_done
    movss xmm0, [rdi]
    mulss xmm0, [rsi]
    addss xmm2, xmm0
    add rdi, 4
    add rsi, 4
    dec rdx
    jmp .Ldot_tail
.Ldot_done:
    movaps xmm0, xmm2
    ret

.section .note.GNU-stack,"",@progbits
