---
title: Branch-Free Code with cmov and setcc
summary: Turn a comparison into a value instead of a jump. cmovcc picks between two registers, setcc stores 0 or 1, and neither can be mispredicted.
minutes: 25
difficulty: medium
tags: flags, branching, instructions
id: asm-intermediate/cmov-and-setcc
mode: harness
---
`cmp` sets the flags; until now you have always followed it with a jump. Two other instruction families read the same flags and **produce data**:

- `cmovcc dst, src` copies `src` to `dst` only if the condition holds (`cmovl`, `cmovge`, `cmovb`, `cmovs`, ...). The source can be a register or memory, never an immediate.
- `setcc r8` writes 1 or 0 into a byte register (`sete al`, `setg cl`, ...). It writes only 8 bits, so clear the full register first.

```asm
# max of two signed values, no jump
mov rax, rdi
cmp rax, rsi
cmovl rax, rsi          # if rax < rsi, take rsi
```

Signed comparisons use `l/le/g/ge`; unsigned ones use `b/be/a/ae` (below/above). Using the wrong family is the classic bug: `-1` is *less* than 1 signed, but `0xFFFF...` is *above* 1 unsigned.

## Gotcha: xor clears the flags

`xor ecx, ecx` is the usual way to get a zero, but it overwrites the flags. Create your zero **before** the `cmp`/`sub` whose result you want to test.

```asm
# sign: -1, 0 or 1
xor eax, eax
xor ecx, ecx
test rdi, rdi
setg al                 # 1 if x > 0
setl cl                 # 1 if x < 0
sub eax, ecx
```
