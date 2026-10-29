Setup: `mov rsi, rdi` (code), `xor ecx, ecx` (pc), `mov r8, rsp` (to restore later). Loop: `movzx eax, byte ptr [rsi + rcx]`, `inc rcx`, `cmp eax, 8`, `ja .Lbad`, `lea rdx, [rip + .Ltable]`, `jmp qword ptr [rdx + rax*8]`. The table holds the nine handler labels in opcode order.
===
Handlers: HALT `pop rax` / `mov rsp, r8` / `ret`. PUSH: `movsx rax, byte ptr [rsi + rcx]` / `inc rcx` / `push rax`. ADD: `pop rax` / `add [rsp], rax`. SUB: `pop rax` / `sub [rsp], rax`. MUL: `pop rax` / `imul rax, [rsp]` / `mov [rsp], rax`. DUP: `push qword ptr [rsp]`. POP: `add rsp, 8`. Every handler (except HALT) ends with `jmp .Lnext`.
===
SWAP: `pop rax` / `pop rdx` / `push rax` / `push rdx`. JNZ: `movzx edx, byte ptr [rsi + rcx]` / `inc rcx` / `pop rax` / `test rax, rax` / `cmovnz rcx, rdx`. Bad opcode: `mov rsp, r8` / `mov rax, -1` / `ret`. Note that `rdx` is free to use here because the dispatch value is not needed after the jump.
