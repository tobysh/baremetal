---
title: Searching PATH
summary: Find the file behind a command name by walking the directories listed in PATH, then run it with execv.
minutes: 30
difficulty: medium
tags: processes, shell, filesystem
id: c-shell/path-search
mode: harness
---
When you type `ls`, nothing says where the program lives. The shell looks it up in the environment variable `PATH`, a colon-separated list of directories such as `/usr/local/bin:/usr/bin:/bin`. It tries each directory in order and runs the **first** match. That is why an earlier directory can shadow a later one.

`execvp` does this for you. A real shell often does it itself so it can print a precise error and distinguish "not found" (exit status 127) from "found but not allowed to run" (126).

## Rules of the search

- If the name contains a `/` (`./tool`, `/usr/bin/ls`) it is a path already: no search.
- Otherwise build `dir + "/" + name` for each directory and test it.
- A match must be a **regular file** (a directory called `tool` does not count) with the **execute** permission for you.

Two library calls cover the tests:

```c
struct stat st;
if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && access(path, X_OK) == 0) { /* usable */ }
```

`access(path, X_OK)` asks the kernel whether *you* may execute the file, which takes ownership and mode bits into account.

## Walking a colon-separated string

Do not use `strtok` (it changes the string). Walk with two pointers: find the next `:` with `strchr`, the directory is the stretch in between, then continue after it. An empty entry (`a::b`) is skipped here to keep things simple.

When the path is built into a caller-provided buffer, use `snprintf` and check that the result fits.

## Your turn

Write `find_in_path` and then `run_with_path`, which uses it with `execv`. Note that `execv` does no searching; the path you give it is used as is.
