---
title: Environment Variables and Expansion
summary: Expand $NAME, ${NAME} and $? inside words, and implement export and unset so that children inherit the changes.
minutes: 30
difficulty: medium
tags: environment, strings, shell
id: c-shell/environment
mode: harness
---
Every process has an **environment**: a list of `NAME=value` strings it inherits from its parent. `PATH`, `HOME` and `LANG` are examples. The C library gives you:

```c
char *home = getenv("HOME");        // NULL if unset
setenv("GREETING", "hi", 1);        // 1 = overwrite if it already exists
unsetenv("GREETING");
```

`fork` copies the environment and `execvp` passes it on, so a variable the shell sets with `setenv` is visible to every program it runs afterwards. That is all that `export FOO=bar` does.

## Expansion

Before running a command, the shell scans each word and replaces **parameter references** with their values:

| Text | Becomes |
| --- | --- |
| `$NAME` | the value of `NAME`, or nothing if unset. A name is a letter or `_` followed by letters, digits and `_` |
| `${NAME}` | the same, but the braces say where the name ends: `${USER}name` |
| `$?` | the exit status of the last command, as a decimal number |
| `$` followed by anything else, or at the end | a literal `$` |

So `x$HOME/y` with `HOME=/root` becomes `x/root/y`, and `$HOMEdir` looks for a variable called `HOMEdir`.

A real shell does this before running the command but after tokenising, which is why quotes of the single kind can stop it. You will work on one already-tokenised word at a time.

## Writing into a bounded buffer

The output can be longer than the input. Append character by character, check the remaining space every time, and return a failure if it does not fit rather than overflowing. Remember that the output must always be a valid C string when you succeed.

## Your turn

Write the expander plus the `export` and `unset` operations.
