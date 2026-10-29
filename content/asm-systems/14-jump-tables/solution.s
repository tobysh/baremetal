.intel_syntax noprefix
.text
.globl apply_op
.globl op_name

.section .rodata
.align 8
.Lops:
    .quad .Ladd, .Lsub, .Lmul, .Land, .Lor, .Lxor, .Lshl, .Lmax
.Lnames:
    .quad .Ns0, .Ns1, .Ns2, .Ns3, .Ns4, .Ns5, .Ns6, .Ns7
.Ns0: .asciz "add"
.Ns1: .asciz "sub"
.Ns2: .asciz "mul"
.Ns3: .asciz "and"
.Ns4: .asciz "or"
.Ns5: .asciz "xor"
.Ns6: .asciz "shl"
.Ns7: .asciz "max"
.Lunknown: .asciz "?"

.text
apply_op:
    cmp rdi, 7
    ja .Ldefault                # unsigned: negative ops land here too
    lea rax, [rip + .Lops]
    jmp qword ptr [rax + rdi*8]
.Ladd:
    lea rax, [rsi + rdx]
    ret
.Lsub:
    mov rax, rsi
    sub rax, rdx
    ret
.Lmul:
    mov rax, rsi
    imul rax, rdx
    ret
.Land:
    mov rax, rsi
    and rax, rdx
    ret
.Lor:
    mov rax, rsi
    or rax, rdx
    ret
.Lxor:
    mov rax, rsi
    xor rax, rdx
    ret
.Lshl:
    mov rax, rsi
    mov rcx, rdx
    shl rax, cl
    ret
.Lmax:
    mov rax, rsi
    cmp rax, rdx
    cmovl rax, rdx
    ret
.Ldefault:
    mov rax, -1
    ret

op_name:
    cmp rdi, 7
    ja .Lunk
    lea rax, [rip + .Lnames]
    mov rax, [rax + rdi*8]
    ret
.Lunk:
    lea rax, [rip + .Lunknown]
    ret

.section .note.GNU-stack,"",@progbits
