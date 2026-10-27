---
title: Pointers and Swap
summary: Memory is a long street of numbered boxes. Learn addresses, the & and * operators, and why a function needs a pointer to change its caller's variable.
minutes: 20
difficulty: medium
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

## Common mistakes

- Using a pointer before it points anywhere (`int *p; *p = 5;`). It holds garbage, so you write to a random house. Always point it at something first.
- Mixing up `p` (the address) and `*p` (the value there).
- Forgetting `&` when calling: `inc(count)` will not compile cleanly, because `count` is an `int`, not an address.

Now write `swap` so it really exchanges two caller variables.
