.intel_syntax noprefix
.text
.globl vm_run

# long vm_run(const unsigned char *code)
# Opcodes: 0 HALT, 1 PUSH n, 2 ADD, 3 SUB, 4 MUL, 5 DUP, 6 SWAP, 7 POP, 8 JNZ t.
# Result is the top of the stack at HALT; unknown opcode returns -1.
vm_run:
    mov rax, -2         # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
