Implement `int run_builtin(char **argv, int *status)`.

`argv` is `NULL`-terminated with at least one word. `*status` holds the status of the previous command on entry. Return:

- `0` if `argv[0]` is not a builtin. Do nothing, leave `*status` alone.
- `1` if it was a builtin and the shell should keep going.
- `2` if the shell should exit.

Builtins:

- `cd`: `cd` alone goes to `$HOME`; `cd dir` goes to `dir`. Success sets `*status = 0`. If `chdir` fails, print `cd: <dir>: <strerror(errno)>` to **stderr** and set `*status = 1`. More than one argument prints `cd: too many arguments` to stderr and sets `*status = 1` (no directory change). If `$HOME` is unset, `cd` alone also fails with status 1.
- `exit`: bare `exit` returns `2` with `*status` unchanged. `exit N` (digits only, optional leading `-`) returns `2` with `*status = N & 255`. `exit abc` prints `exit: abc: numeric argument required` to stderr, sets `*status = 2` and returns `1` (the shell stays alive). More than one argument prints `exit: too many arguments`, sets `*status = 1` and returns `1`.
