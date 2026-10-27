---
title: Loops and Memory
summary: Read an array from memory with pointers, addressing modes and a counted loop, and handle the empty array correctly.
minutes: 20
difficulty: medium
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

## Types matter

`size_t` is an unsigned 64-bit type, so compare it with unsigned jumps (`jae`, `jb`). A pointer to `long` arrives in `rdi`, the count in `rsi`.

## Common mistakes

- Forgetting the `*8`: `[rdi + rcx]` reads from the middle of an element.
- Off-by-one: loop while `i < n`, not `i <= n`.
- Reading before checking for an empty array.
- Using `mov rax, rdi` when you meant `mov rax, [rdi]`. Without brackets you get the address, not the value.

Now implement `sum_array`.
