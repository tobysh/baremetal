Implement `int run_pipe2(char **left, char **right, int out_fd)`.

Run `left | right`. The standard output of `left` goes into the pipe, the standard input of `right` comes from it, and the standard output of `right` is `out_fd` (or the shell's own stdout if `out_fd` is `-1`). Both commands run at the same time.

- Use `pipe`, `fork`, `dup2` and `execvp`. A command that cannot be started exits with 127.
- Return the exit code of `right` (`WEXITSTATUS`; return `128 + signal` if it was killed by one). The exit code of `left` is ignored.
- Wait for **both** children, leave no zombies, and leave no file descriptor open in the parent (do not close `out_fd`; it belongs to the caller).
- If `pipe` or `fork` fails, return `-1`.
