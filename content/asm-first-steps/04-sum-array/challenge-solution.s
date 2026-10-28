.intel_syntax noprefix
.text
.globl max_array

# long max_array(const long *a, size_t n)
max_array:
    mov rax, [rdi]          # best so far = a[0]
    mov rcx, 1              # i = 1
.loop:
    cmp rcx, rsi
    jae .done               # i >= n (unsigned: n is a count)
    mov rdx, [rdi + rcx*8]
    cmp rdx, rax
    jle .next               # signed compare
    mov rax, rdx
.next:
    inc rcx
    jmp .loop
.done:
    ret

.section .note.GNU-stack,"",@progbits
