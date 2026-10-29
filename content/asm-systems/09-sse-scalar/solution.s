.intel_syntax noprefix
.text
.globl distance
.globl mean
.globl floor_to_long

distance:
    subsd xmm0, xmm2
    subsd xmm1, xmm3
    mulsd xmm0, xmm0
    mulsd xmm1, xmm1
    addsd xmm0, xmm1
    sqrtsd xmm0, xmm0
    ret

mean:
    pxor xmm0, xmm0
    test rsi, rsi
    jz .Lmean_done
    xor ecx, ecx
.Lmean_loop:
    cvtsi2sd xmm1, qword ptr [rdi + rcx*8]
    addsd xmm0, xmm1
    inc rcx
    cmp rcx, rsi
    jb .Lmean_loop
    cvtsi2sd xmm1, rsi
    divsd xmm0, xmm1
.Lmean_done:
    ret

floor_to_long:
    cvttsd2si rax, xmm0
    cvtsi2sd xmm1, rax
    ucomisd xmm0, xmm1
    jae .Lfloor_done
    dec rax
.Lfloor_done:
    ret

.section .note.GNU-stack,"",@progbits
