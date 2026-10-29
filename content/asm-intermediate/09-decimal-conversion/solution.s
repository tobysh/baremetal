.intel_syntax noprefix
.text
.globl u64_to_dec
.globl parse_dec

u64_to_dec:
    mov rax, rdi
    mov r8, rsi
    mov r9, rsi
    mov r10d, 10
.Ld_dig:
    xor edx, edx
    div r10
    add dl, 48
    mov [r9], dl
    inc r9
    test rax, rax
    jnz .Ld_dig
    mov byte ptr [r9], 0
    mov rax, r9
    sub rax, r8
    lea rcx, [r9 - 1]
.Ld_rev:
    cmp r8, rcx
    jae .Ld_done
    mov dl, [r8]
    mov r11b, [rcx]
    mov [r8], r11b
    mov [rcx], dl
    inc r8
    dec rcx
    jmp .Ld_rev
.Ld_done:
    ret

parse_dec:
    xor eax, eax
    xor r8d, r8d
    movzx ecx, byte ptr [rdi]
    cmp ecx, 45
    jne .Lp_plus
    mov r8d, 1
    inc rdi
    jmp .Lp_digits
.Lp_plus:
    cmp ecx, 43
    jne .Lp_digits
    inc rdi
.Lp_digits:
    movzx ecx, byte ptr [rdi]
    sub ecx, 48
    cmp ecx, 9
    ja .Lp_end
    lea rax, [rax + rax*4]
    lea rax, [rcx + rax*2]
    inc rdi
    jmp .Lp_digits
.Lp_end:
    test r8d, r8d
    jz .Lp_ret
    neg rax
.Lp_ret:
    ret

.section .note.GNU-stack,"",@progbits
