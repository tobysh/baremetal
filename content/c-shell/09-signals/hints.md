`setup_signals`: fill a `struct sigaction sa` (zero it first), call `sigemptyset(&sa.sa_mask)`, set `sa.sa_handler = on_sigint` and `sa.sa_flags = SA_RESTART`, then `sigaction(SIGINT, &sa, NULL)`. For SIGQUIT set `sa.sa_handler = SIG_IGN` and call `sigaction` again.
===
`run_foreground` is the usual fork/exec/wait, with two calls in the child before `execvp`: `signal(SIGINT, SIG_DFL); signal(SIGQUIT, SIG_DFL);` (or `sigaction`).
===
Without the reset, `sh -c 'kill -QUIT $$'` is not killed because the child inherited "ignore". The wait status then says it exited normally with 0 instead of being signalled.
