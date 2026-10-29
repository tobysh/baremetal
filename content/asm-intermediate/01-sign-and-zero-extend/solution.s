.intel_syntax noprefix
.text
.globl sum_i8
.globl sum_u8
.globl sum_i32

sum_i8:
    xor eax, eax
    xor ecx, ecx
.L8s:
    cmp rcx, rsi
    jae .L8sd
    movsx rdx, byte ptr [rdi + rcx]
    add rax, rdx
    inc rcx
    jmp .L8s
.L8sd:
    ret

sum_u8:
    xor eax, eax
    xor ecx, ecx
.L8u:
    cmp rcx, rsi
    jae .L8ud
    movzx edx, byte ptr [rdi + rcx]
    add rax, rdx
    inc rcx
    jmp .L8u
.L8ud:
    ret

sum_i32:
    xor eax, eax
    xor ecx, ecx
.L32:
    cmp rcx, rsi
    jae .L32d
    movsxd rdx, dword ptr [rdi + rcx*4]
    add rax, rdx
    inc rcx
    jmp .L32
.L32d:
    ret

.section .note.GNU-stack,"",@progbits
