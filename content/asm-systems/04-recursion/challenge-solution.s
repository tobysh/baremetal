.intel_syntax noprefix
.text
.globl binom_rec
.globl ackermann

binom_rec:
    cmp rsi, rdi
    ja .Lzero               # k > n
    test rsi, rsi
    jz .Lone                # k == 0
    cmp rsi, rdi
    je .Lone                # k == n
    sub rsp, 24             # 8 (return address) + 24 keeps rsp 16-aligned at the calls
    mov [rsp], rdi
    mov [rsp + 8], rsi
    dec rdi
    dec rsi
    call binom_rec          # C(n-1, k-1)
    mov [rsp + 16], rax
    mov rdi, [rsp]
    dec rdi
    mov rsi, [rsp + 8]
    call binom_rec          # C(n-1, k)
    add rax, [rsp + 16]
    add rsp, 24
    ret
.Lone:
    mov eax, 1
    ret
.Lzero:
    xor eax, eax
    ret

ackermann:
    test rdi, rdi
    jnz .Lm_nonzero
    lea rax, [rsi + 1]
    ret
.Lm_nonzero:
    test rsi, rsi
    jnz .Lboth
    dec rdi
    mov esi, 1
    jmp ackermann           # tail call: A(m-1, 1)
.Lboth:
    push rdi                # also re-aligns rsp for the call
    dec rsi
    call ackermann          # A(m, n-1)
    mov rsi, rax
    pop rdi
    dec rdi
    jmp ackermann           # tail call: A(m-1, result)

.section .note.GNU-stack,"",@progbits
