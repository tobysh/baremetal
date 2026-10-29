.intel_syntax noprefix
.text
.globl total_score
.globl heaviest

total_score:
    xor eax, eax
    xor ecx, ecx
.Lts_loop:
    cmp rcx, rsi
    jae .Lts_done
    mov edx, [rdi]
    test edx, edx
    jle .Lts_skip
    movsxd rdx, edx
    imul rdx, [rdi + 8]
    add rax, rdx
    movsxd rdx, dword ptr [rdi + 4]
    add rax, rdx
.Lts_skip:
    add rdi, 16
    inc rcx
    jmp .Lts_loop
.Lts_done:
    ret

heaviest:
    test rsi, rsi
    jnz .Lh_start
    mov rax, -1
    ret
.Lh_start:
    xor eax, eax                # best = 0
    mov r8d, [rdi]              # best weight
    mov rcx, 1
.Lh_loop:
    cmp rcx, rsi
    jae .Lh_done
    mov rdx, rcx
    shl rdx, 4
    mov r9d, [rdi + rdx]
    cmp r9d, r8d
    jle .Lh_next
    mov r8d, r9d
    mov rax, rcx
.Lh_next:
    inc rcx
    jmp .Lh_loop
.Lh_done:
    ret

.section .note.GNU-stack,"",@progbits
