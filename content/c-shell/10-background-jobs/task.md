Implement two functions. The starter declares `volatile sig_atomic_t reaped`.

- `void install_reaper(void)`: install a `SIGCHLD` handler with `sigaction` using `SA_RESTART | SA_NOCLDSTOP`. The handler reaps **all** children that have finished (loop with `waitpid(-1, NULL, WNOHANG)`), adds the number reaped to `reaped`, and leaves `errno` unchanged.
- `pid_t run_background(char *const argv[])`: start the command with `execvp` in a child (which exits 127 if exec fails) and return the child's pid immediately, **without waiting**. Return `-1` if `fork` fails.
