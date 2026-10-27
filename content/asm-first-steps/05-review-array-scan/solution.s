.intel_syntax noprefix
.text
.globl sum_clamped
.globl count_negative

# long sum_clamped(const long *a, size_t n, long lo, long hi)
sum_clamped:
    xor eax, eax            # total = 0
    xor r8d, r8d            # i = 0
.Lsc_loop:
    cmp r8, rsi
    jae .Lsc_done           # i >= n (unsigned)
    mov r9, [rdi + r8*8]    # x = a[i]
    cmp r9, rdx
    jge .Lsc_not_low        # x >= lo (signed)
    mov r9, rdx             # x = lo
.Lsc_not_low:
    cmp r9, rcx
    jle .Lsc_not_high       # x <= hi (signed)
    mov r9, rcx             # x = hi
.Lsc_not_high:
    add rax, r9
    inc r8
    jmp .Lsc_loop
.Lsc_done:
    ret

# size_t count_negative(const long *a, size_t n)
count_negative:
    xor eax, eax            # count = 0
    xor ecx, ecx            # i = 0
.Lcn_loop:
    cmp rcx, rsi
    jae .Lcn_done
    cmp qword ptr [rdi + rcx*8], 0
    jge .Lcn_skip           # element >= 0 (signed)
    inc rax
.Lcn_skip:
    inc rcx
    jmp .Lcn_loop
.Lcn_done:
    ret

.section .note.GNU-stack,"",@progbits
