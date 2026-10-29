.intel_syntax noprefix
.text
.globl clamp_byte
.globl count_in_range
.globl nibble_at

clamp_byte:
    xor ecx, ecx
    mov edx, 255
    mov rax, rdi
    test rax, rax
    cmovs rax, rcx
    cmp rax, rdx
    cmovg rax, rdx
    ret

count_in_range:
    xor eax, eax
    xor r9d, r9d
.Lcr:
    cmp r9, rsi
    jae .Lcr_done
    movsx r8d, byte ptr [rdi + r9]
    cmp r8d, edx
    setge r10b
    cmp r8d, ecx
    setle r11b
    and r10b, r11b
    movzx r10d, r10b
    add rax, r10
    inc r9
    jmp .Lcr
.Lcr_done:
    ret

nibble_at:
    lea ecx, [rsi*4]
    shr rdi, cl
    mov eax, edi
    and eax, 15
    ret

.section .note.GNU-stack,"",@progbits
