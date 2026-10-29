Implement three functions. The starter declares the global `volatile sig_atomic_t sigint_seen`.

- `void setup_signals(void)`: install a `SIGINT` handler that sets `sigint_seen = 1` (use `sigaction` with `SA_RESTART`), and set `SIGQUIT` to be ignored.
- `int take_interrupt(void)`: return `1` and clear the flag if `sigint_seen` was set, otherwise `0`.
- `int run_foreground(char *const argv[])`: run the command in a child (`execvp`) and wait for it. In the child, restore the default action for `SIGINT` and `SIGQUIT` before `exec`. Return the exit code, `128 + signal` if it was killed by a signal, `127` if it cannot be started, and `-1` if `fork` fails. Retry `waitpid` on `EINTR`.
