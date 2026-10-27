---
title: Loops and Memory
summary: Read an array from memory with pointers, addressing modes and a counted loop, and handle the empty array correctly.
minutes: 20
difficulty: medium
tags: loops, memory, arrays
id: asm-first-steps/sum-array
mode: harness
---
So far everything lived in registers. Arrays live in **memory**, and a pointer is just a number: the address of the first byte.

## Reading and writing memory

Square brackets mean "the memory at this address", like `*p` in C:

```asm
mov rax, [rdi]        # rax = *rdi          (load 8 bytes)
mov [rdi], rax        # *rdi = rax          (store 8 bytes)
add rax, [rdi]        # rax += *rdi
```

The size comes from the register: `rax` is 64 bits, so it moves 8 bytes. A `long` is 8 bytes, so `[rdi]` is exactly one `long`.

### How memory is laid out

Memory is one long row of **bytes**, each with its own address. A `long` takes 8 of them, so an array of three `long`s occupies 24 consecutive bytes:

```
address:   a     a+8    a+16
           [ a[0] ][ a[1] ][ a[2] ]      each cell is 8 bytes
```

x86 is **little-endian**: inside one 8-byte value, the *least significant* byte is stored at the lowest address. The number `1` is stored as `01 00 00 00 00 00 00 00`. You rarely notice this, because `mov rax, [rdi]` reassembles the bytes for you, but it matters when you read a `long` as individual bytes or when you look at a memory dump.

The consequence for arrays is simple: element `i` starts `i * element_size` bytes after the start. That multiplication is exactly what the addressing mode below does for you.

## Addressing modes

The brackets can hold a small expression: `[base + index*scale + offset]`, where scale is 1, 2, 4 or 8:

```asm
mov rax, [rdi + rcx*8]       # rax = a[i]   when rdi = a and rcx = i
mov rax, [rdi + 8]           # rax = a[1]
lea rdx, [rdi + rcx*8]       # rdx = &a[i]  (computes the address, reads nothing)
```

The scale is the element size, so arrays of `long` use `*8`. `lea` ("load effective address") does the address maths without touching memory; it is also a handy three-operand add.

## A counted loop

A `for` loop is a label at the top, the body, and a conditional jump back:

```asm
    xor eax, eax          # total = 0 (writing eax also zeroes the top half of rax)
    xor ecx, ecx          # i = 0
.Lloop:
    cmp rcx, rsi          # i < n ?
    jae .Ldone            # unsigned: n is a size_t
    add rax, [rdi + rcx*8]
    inc rcx
    jmp .Lloop
.Ldone:
    ret
```

`xor eax, eax` is the traditional way to set a register to zero: it is short and fast. `inc` adds one.

Checking the condition at the **top** makes the loop correct for `n = 0`: the body never runs. If you put the check at the bottom, an empty array would read memory it should not.

### Tracing the loop

For `sum_array` on the array `{3, 4, 5}` with `n = 3`, the loop state at each check of the condition is:

| Iteration | `rcx` (i) | `rax` (total) | what happens |
|---|---|---|---|
| 1 | 0 | 0 | 0 < 3, add `a[0]` = 3 |
| 2 | 1 | 3 | 1 < 3, add `a[1]` = 4 |
| 3 | 2 | 7 | 2 < 3, add `a[2]` = 5 |
| exit | 3 | 12 | 3 < 3 is false, jump to `.Ldone`, return 12 |

For `n = 0` the very first check is `0 < 0`, which is false, so the function returns the 0 it set up at the start without reading memory at all. That is the whole reason the test sits at the top.

### When the size is not obvious

Whenever a register is involved, the operand size is clear. When you write to memory with a constant and no register, it is not:

```asm
mov [rdi], 5                 # error: how many bytes?
mov qword ptr [rdi], 5       # store 8 bytes
mov dword ptr [rdi], 5       # store 4 bytes
```

`qword` is "quad word", 8 bytes; `dword` is 4, `word` is 2, `byte` is 1. You do not need this to solve the exercise, but you will meet the error message as soon as you try to store a constant.

## Types matter

`size_t` is an unsigned 64-bit type, so compare it with unsigned jumps (`jae`, `jb`). A pointer to `long` arrives in `rdi`, the count in `rsi`.

## Common mistakes

- Forgetting the `*8`: `[rdi + rcx]` reads from the middle of an element.
- Off-by-one: loop while `i < n`, not `i <= n`.
- Reading before checking for an empty array.
- Using `mov rax, rdi` when you meant `mov rax, [rdi]`. Without brackets you get the address, not the value.

Now implement `sum_array`.
