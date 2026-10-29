.intel_syntax noprefix
.text
.globl vm_run

.section .rodata
.align 8
.Ltable:
    .quad .Lhalt, .Lpush, .Ladd, .Lsub, .Lmul, .Ldup, .Lswap, .Lpop, .Ljnz

.text
vm_run:
    mov rsi, rdi                # code
    xor ecx, ecx                # pc
    mov r8, rsp                 # entry stack pointer, restored on every exit
.Lnext:
    movzx eax, byte ptr [rsi + rcx]
    inc rcx
    cmp eax, 8
    ja .Lbad
    lea rdx, [rip + .Ltable]
    jmp qword ptr [rdx + rax*8]
.Lhalt:
    pop rax
    mov rsp, r8
    ret
.Lpush:
    movsx rax, byte ptr [rsi + rcx]
    inc rcx
    push rax
    jmp .Lnext
.Ladd:
    pop rax
    add [rsp], rax
    jmp .Lnext
.Lsub:
    pop rax
    sub [rsp], rax
    jmp .Lnext
.Lmul:
    pop rax
    imul rax, [rsp]
    mov [rsp], rax
    jmp .Lnext
.Ldup:
    push qword ptr [rsp]
    jmp .Lnext
.Lswap:
    pop rax
    pop rdx
    push rax
    push rdx
    jmp .Lnext
.Lpop:
    add rsp, 8
    jmp .Lnext
.Ljnz:
    movzx edx, byte ptr [rsi + rcx]
    inc rcx
    pop rax
    test rax, rax
    cmovnz rcx, rdx
    jmp .Lnext
.Lbad:
    mov rsp, r8
    mov rax, -1
    ret

.section .note.GNU-stack,"",@progbits
