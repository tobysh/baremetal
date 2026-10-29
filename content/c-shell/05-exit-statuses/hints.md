`exit_code`: `if (WIFEXITED(s)) return WEXITSTATUS(s); if (WIFSIGNALED(s)) return 128 + WTERMSIG(s);` and similar for `WIFSTOPPED`. Check for `-1` first, because the macros on `-1` give nonsense.
===
`run_list`: `int sa = exit_code(run_wait(a));` then decide whether `b` should run. A single `if` per operator is enough. If it should not run, return `sa`.
