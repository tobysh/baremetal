/* Hidden boot stub for the "kernel" language. QEMU loads this as a Multiboot 1
 * kernel in 32-bit protected mode; we switch to 64-bit long mode, identity-map
 * the first GiB, and call the learner's kmain(). */
.intel_syntax noprefix

.section .multiboot, "a"
.align 4
.long 0x1BADB002                 /* magic */
.long 0x00000003                 /* flags: page-align modules, give us memory info */
.long -(0x1BADB002 + 0x00000003) /* checksum */

.section .boot, "ax"
.code32
.globl _start
_start:
    cli
    mov esp, offset boot_stack_top
    mov [bm_multiboot_info], ebx
    mov [bm_multiboot_magic], eax
    /* identity-map 0..1 GiB with 2 MiB pages */
    mov eax, offset pdpt
    or eax, 3
    mov [pml4], eax
    mov eax, offset pd
    or eax, 3
    mov [pdpt], eax
    xor ecx, ecx
1:  mov eax, ecx
    shl eax, 21
    or eax, 0x83
    mov [pd + ecx*8], eax
    inc ecx
    cmp ecx, 512
    jne 1b
    /* the first 2 MiB uses 4 KiB pages so page 0 can stay unmapped: a null dereference faults */
    mov eax, offset pt
    or eax, 3
    mov [pd], eax
    mov ecx, 1
4:  mov eax, ecx
    shl eax, 12
    or eax, 3
    mov [pt + ecx*8], eax
    inc ecx
    cmp ecx, 512
    jne 4b
    mov eax, offset pml4
    mov cr3, eax
    mov eax, cr4
    or eax, 0x20                 /* PAE */
    mov cr4, eax
    mov ecx, 0xC0000080          /* EFER */
    rdmsr
    or eax, 0x100                /* LME */
    wrmsr
    mov eax, cr0
    or eax, 0x80000001           /* PG | PE */
    mov cr0, eax
    lgdt [boot_gdt_ptr]
    .byte 0xea                   /* ljmp 0x08:long_mode */
    .long long_mode
    .word 0x08

.code64
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    mov rsp, offset boot_stack_top
    xor ebp, ebp
    call bm_runtime_init
    call kmain
    xor edi, edi
    call bm_exit
2:  hlt
    jmp 2b

/* Exception stubs: push a vector (and a dummy error code where the CPU pushes none). */
.macro ISR n
isr\n:
    push 0
    push \n
    jmp isr_common
.endm
.macro ISR_ERR n
isr\n:
    push \n
    jmp isr_common
.endm

ISR 0; ISR 1; ISR 2; ISR 3; ISR 4; ISR 5; ISR 6; ISR 7
ISR_ERR 8; ISR 9; ISR_ERR 10; ISR_ERR 11; ISR_ERR 12; ISR_ERR 13; ISR_ERR 14; ISR 15
ISR 16; ISR_ERR 17; ISR 18; ISR 19; ISR 20; ISR_ERR 21; ISR 22; ISR 23
ISR 24; ISR 25; ISR 26; ISR 27; ISR 28; ISR_ERR 29; ISR_ERR 30; ISR 31
ISR 32; ISR 33; ISR 34; ISR 35; ISR 36; ISR 37; ISR 38; ISR 39
ISR 40; ISR 41; ISR 42; ISR 43; ISR 44; ISR 45; ISR 46; ISR 47

/* Saves every register, calls bm_dispatch(vector, error code, rip, cr2), restores, iretq. */
isr_common:
    push rax; push rcx; push rdx; push rbx; push rbp; push rsi; push rdi
    push r8; push r9; push r10; push r11; push r12; push r13; push r14; push r15
    mov rdi, [rsp + 15*8]        /* vector */
    mov rsi, [rsp + 16*8]        /* error code */
    mov rdx, [rsp + 17*8]        /* rip */
    mov rcx, cr2
    cld
    call bm_dispatch
    pop r15; pop r14; pop r13; pop r12; pop r11; pop r10; pop r9; pop r8
    pop rdi; pop rsi; pop rbp; pop rbx; pop rdx; pop rcx; pop rax
    add rsp, 16                  /* vector + error code */
    iretq

.section .rodata
.align 8
.globl bm_isr_table
bm_isr_table:
.irp n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
    .quad isr\n
.endr

.align 8
boot_gdt:
    .quad 0
    .quad 0x00AF9A000000FFFF     /* 0x08: 64-bit code */
    .quad 0x00CF92000000FFFF     /* 0x10: data */
boot_gdt_ptr:
    .word boot_gdt_ptr - boot_gdt - 1
    .long boot_gdt

.section .bss
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pd:   .skip 4096
pt:   .skip 4096
.skip 65536
boot_stack_top:
.globl bm_multiboot_info, bm_multiboot_magic
bm_multiboot_info:  .skip 4
bm_multiboot_magic: .skip 4
