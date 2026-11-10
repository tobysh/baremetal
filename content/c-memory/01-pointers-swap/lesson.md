---
title: Pointers and Swap
summary: Memory is a long street of numbered boxes. Learn addresses, the & and * operators, and why a function needs a pointer to change its caller's variable.
minutes: 20
difficulty: medium
tags: pointers
id: c-memory/pointers-swap
mode: harness
---
Your computer's **memory** (RAM) is a huge row of tiny boxes called **bytes**. Think of a very long street of houses. Every house holds a small amount of data, and every house has a number painted on the door: its **address**. House 0, house 1, house 2, and so on, up into the billions.

When you write `int x = 42;`, the compiler reserves a few neighbouring houses (an `int` usually takes 4 bytes) and puts 42 inside. The variable name `x` is just a friendly label for that spot.

## Getting an address with &

The `&` operator ("address of") gives you the street address instead of the contents:

```c
int x = 42;
printf("value:   %d\n", x);
printf("address: %p\n", (void *)&x);   // something like 0x7ffd5c3a1b4c
```

(`%p` prints an address. The exact number changes from run to run; that is normal.)

## Storing an address: pointers

A **pointer** is a variable whose value is an address. Its type says what kind of thing lives at that address:

```c
int x = 42;
int *p = &x;    // p holds the address of x. "p points to x."
```

`int *p` reads as "p is a pointer to int".

## Following an address with *

The `*` operator ("dereference") means "go to that address and use what is there":

```c
printf("%d\n", *p);   // 42, the value at the address
*p = 99;              // go to the address and write 99 there
printf("%d\n", x);    // 99! x changed, because p points at x
```

Note that `*` does two jobs: in a declaration (`int *p`) it means "this is a pointer", and in an expression (`*p`) it means "follow the pointer".

## What's in memory

A pointer is an ordinary box. The only special thing is what it holds: the address of another box. After `int x = 42; int *p = &x;` the frame of `main` looks like this (addresses are made up):

```text
main's frame
0x7000   x   42
0x7008   p   0x7000      <- p holds x's address
```

`*p` means "go to address 0x7000 and use the box there". Writing `*p = 99;` changes the box at `0x7000`, which is `x`.

## Why swap(int a, int b) cannot work

C passes arguments **by value**: each function gets its own *copy*.

```c
void bad_swap(int a, int b) {
    int tmp = a;
    a = b;
    b = tmp;        // only the copies are swapped
}
```

After `bad_swap(x, y)`, your `x` and `y` are untouched. The function swapped two photocopies.

The fix is to hand over the *addresses* instead. The function gets copies of the addresses, but a copy of an address still leads to the same house:

```c
void inc(int *n) {
    *n = *n + 1;
}

int main(void) {
    int count = 5;
    inc(&count);            // pass the address
    printf("%d\n", count);  // 6
    return 0;
}
```

## What's in memory: calling inc(&count)

A call pushes a new frame on top of the caller's. The argument is copied into the new frame as usual, but what is copied is an address, so it still leads to the caller's box:

```text
while inc runs      inc's frame     n      0x7000   <- a copy of the address
                    main's frame    count  5        at address 0x7000

after *n = *n + 1                   main's frame    count  6

after inc returns   main's frame    count  6        (inc's frame is gone, n with it)
```

The pointer `n` lives only as long as `inc`'s frame does. The box it pointed at, `count`, lives in `main`'s frame, which is why the change survives. The reverse is the trap: a pointer to a local variable is only good while that variable's frame exists. The heap lesson shows what goes wrong when a function returns such a pointer.

## Common mistakes

- Using a pointer before it points anywhere (`int *p; *p = 5;`). It holds garbage, so you write to a random house. Always point it at something first.
- Mixing up `p` (the address) and `*p` (the value there).
- Forgetting `&` when calling: `inc(count)` will not compile cleanly, because `count` is an `int`, not an address.

Now write `swap` so it really exchanges two caller variables.
