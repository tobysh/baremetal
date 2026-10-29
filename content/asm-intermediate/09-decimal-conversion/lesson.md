---
title: Decimal Conversion
summary: Turn a number into text with repeated division by 10, then reverse it; turn text into a number with multiply-by-10-and-add. The core of printf and atoi.
minutes: 40
difficulty: hard
tags: strings, arithmetic, loops
id: asm-intermediate/decimal-conversion
mode: harness
---
Computers hold numbers in binary; people read decimal. Both conversions are small loops you now have every tool for.

## Number to text

Divide by 10 repeatedly. The **remainders** are the digits, **least significant first**:

```
1203 / 10 = 120 r 3
 120 / 10 =  12 r 0
  12 / 10 =   1 r 2
   1 / 10 =   0 r 1        digits produced: 3 0 2 1
```

So write each remainder plus `'0'` (48) into the buffer as it comes, then **reverse the buffer in place** with two cursors moving toward each other. Use `do { ... } while (n)` logic (test at the bottom) so that `0` still produces one digit.

```asm
mov r10d, 10
.Ldig:
    xor edx, edx
    div r10                 # rax = quotient, rdx = digit
    add dl, 48
    mov [r9], dl
    inc r9
    test rax, rax
    jnz .Ldig
```

## Text to number

Process left to right: `value = value*10 + digit`. A trick to validate a digit in one comparison: `sub ecx, 48` then `cmp ecx, 9` / `ja notdigit`. Characters below `'0'` wrap around to huge unsigned values, so one unsigned compare rejects both sides. Multiply by 10 with two `lea`s:

```asm
lea rax, [rax + rax*4]      # *5
lea rax, [rcx + rax*2]      # *10 + digit
```

A leading `-` negates the final result (`neg rax`).
