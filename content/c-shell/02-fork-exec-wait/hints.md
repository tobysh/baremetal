Call `fork()`. In the child (`pid == 0`) call `execvp(argv[0], argv)` and, if that returns, `_exit(127)`.
===
In the parent, loop on `waitpid(pid, &status, 0)` while it returns `-1` with `errno == EINTR`. Then use `WIFEXITED(status)` to decide between `WEXITSTATUS(status)` and `-1`.
