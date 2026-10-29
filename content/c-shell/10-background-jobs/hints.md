`run_background` is `fork` plus `execvp` and `_exit(127)` in the child, then `return pid;` in the parent. There is no `waitpid` at all.
===
The handler: save `errno`, then `while (waitpid(-1, NULL, WNOHANG) > 0) reaped++;`, then restore `errno`. Fill in a `struct sigaction` with `sa_handler`, `sigemptyset(&sa.sa_mask)` and the two flags.
