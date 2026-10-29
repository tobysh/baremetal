Implement `int run_pipeline(char ***cmds, int n, int in_fd, int out_fd)`.

`cmds[0] ... cmds[n-1]` are `NULL`-terminated `argv` arrays (`n >= 1`). Run them all at the same time, each one's stdout feeding the next one's stdin.

- If `in_fd >= 0`, it becomes the stdin of the first command; if `-1`, the first command inherits the shell's stdin.
- If `out_fd >= 0`, it becomes the stdout of the last command; if `-1`, it inherits the shell's stdout.
- `in_fd` and `out_fd` belong to the caller: do not close them in the parent.
- Return the exit code of the **last** command (`128 + signal` if it was killed by one). A command that cannot be started exits with 127.
- Wait for every child, leave no zombies and no descriptors open in the parent.
- If `pipe` or `fork` fails, clean up as well as you can and return `-1`.
