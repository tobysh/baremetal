Implement `long vm_run(const unsigned char *code)`: execute the bytecode starting at `code[0]` using the instruction set from the lesson, and return the value on top of the stack when `HALT` is reached. Opcodes: 0 `HALT`, 1 `PUSH n` (signed byte), 2 `ADD`, 3 `SUB` (second-from-top minus top), 4 `MUL`, 5 `DUP`, 6 `SWAP`, 7 `POP`, 8 `JNZ t` (pop; jump to absolute offset `t` if non-zero). An unknown opcode returns `-1`.

`code` is in `rdi`. Dispatch through a jump table. Whatever you do with `rsp`, it must be exactly the entry value when you `ret`. Preserve `rbx`, `rbp` and `r12`-`r15`.
