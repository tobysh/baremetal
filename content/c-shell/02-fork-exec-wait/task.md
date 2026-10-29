Implement `int run_command(char *const argv[])`. `argv` is a `NULL`-terminated array with the program name in `argv[0]`.

- Start the program in a child process (using `PATH` to find it, so `execvp`) and wait for that child.
- Return the child's exit code (0 to 255) if it exited normally.
- Return `127` if the program could not be started (for example it does not exist).
- Return `-1` if `fork` fails, or if the child was killed by a signal (the next lessons improve on this).
- Retry `waitpid` if it is interrupted. When you return, no zombie child may be left behind.
