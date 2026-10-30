---
title: Pointers to Pointers and Out-Parameters
summary: Use a pointer to a pointer so a function can hand back a new string or move the caller's cursor, and see how out-parameters return extra results.
minutes: 25
difficulty: medium
tags: pointers, out-parameters, strings
id: c-memory/pointers-to-pointers
mode: harness
---
In the first lesson a function changed its caller's `int` by receiving a pointer to it. The rule was: to change something in the caller, pass the address of that something.

That rule works for every type, including pointers. Suppose a function should change which string the caller's `char *` points at. The thing to change is a `char *`, so the function needs a pointer to a `char *`: a `char **`.

```c
void point_at_hello(const char **out) {
    *out = "hello";        // changes the caller's pointer
}

const char *s = NULL;
point_at_hello(&s);        // s now points at "hello"
```

Read `char **out` right to left: `out` is a pointer to a pointer to a `char`. `*out` is the caller's pointer; `**out` would be the first character.

## Out-parameters

A C function returns only one value. When it needs to give back something more (a new string, a second number), it takes an **out-parameter**: a pointer where it writes the extra result. The return value is then free to report success or failure:

```c
int make_label(int id, char **out) {
    char *buf = malloc(16);
    if (buf == NULL) {
        *out = NULL;
        return -1;            // failure
    }
    snprintf(buf, 16, "item-%d", id);
    *out = buf;               // hand the new string to the caller
    return 0;                 // success
}

char *label;
if (make_label(7, &label) == 0) {
    printf("%s\n", label);
    free(label);              // the caller owns it now
}
```

Two habits keep this safe. Always set `*out` on every path (to `NULL` on failure), so the caller never reads an uninitialised pointer. And document who frees: here the caller does.

## Moving the caller's cursor

The same trick lets a function advance a pointer that the caller is using to walk through text. This is how parsers work: each call consumes a piece of the input and moves the cursor past it.

```c
void skip_digits(const char **p) {
    while (**p >= '0' && **p <= '9') {
        (*p)++;
    }
}
```

Note the parentheses in `(*p)++`. Without them, `*p++` means `*(p++)`, which moves the local `p` instead of the caller's pointer. This is the classic mistake.

## Common mistakes

- Assigning to `out` instead of `*out`. That only changes the function's own copy.
- Forgetting the parentheses in `(*p)++`.
- Leaving `*out` unset when the function fails.

## Your turn

Write three functions: one that moves a cursor, one that returns a new string through `char **`, and one that combines both.
