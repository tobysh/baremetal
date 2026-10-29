.intel_syntax noprefix
.text
.globl get_bits
.globl rotl32
.globl set_bits

get_bits:
    mov ecx, esi
    shr rdi, cl
    mov ecx, edx
    mov eax, 1
    shl rax, cl
    dec rax
    and rax, rdi
    ret

rotl32:
    mov ecx, esi
    mov eax, edi
    rol eax, cl
    ret

set_bits:
    mov r8, rcx
    mov ecx, edx
    mov eax, 1
    shl rax, cl
    dec rax
    and r8, rax
    mov ecx, esi
    shl rax, cl
    shl r8, cl
    not rax
    and rdi, rax
    or rdi, r8
    mov rax, rdi
    ret

.section .note.GNU-stack,"",@progbits
