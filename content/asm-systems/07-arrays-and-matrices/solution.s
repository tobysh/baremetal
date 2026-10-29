.intel_syntax noprefix
.text
.globl prefix_sums
.globl trace

prefix_sums:
    xor eax, eax                # running total
    xor ecx, ecx                # i
.Lps_loop:
    cmp rcx, rsi
    jae .Lps_done
    add rax, [rdi + rcx*8]
    mov [rdi + rcx*8], rax
    inc rcx
    jmp .Lps_loop
.Lps_done:
    ret

trace:
    xor eax, eax
    xor ecx, ecx
    lea r8, [rsi + 1]
    shl r8, 2                   # stride in bytes: (n + 1) * 4
.Ltr_loop:
    cmp rcx, rsi
    jae .Ltr_done
    movsxd rdx, dword ptr [rdi]
    add rax, rdx
    add rdi, r8
    inc rcx
    jmp .Ltr_loop
.Ltr_done:
    ret

.section .note.GNU-stack,"",@progbits
