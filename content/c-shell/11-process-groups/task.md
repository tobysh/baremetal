Implement three functions.

- `pid_t spawn_group(char *const argv[])`: fork a child that becomes the leader of a **new process group** and runs the command with `execvp` (`_exit(127)` if that fails). Both the child and the parent must call `setpgid`, so that when `spawn_group` returns in the parent, `getpgid(child) == child` already holds. Return the child's pid (which is also the group id), or `-1` if `fork` fails.
- `int kill_group(pid_t pgid, int sig)`: send `sig` to every process in the group; return `0` on success and `-1` on failure.
- `int wait_job(pid_t pid, int *stopped)`: wait for `pid` using `WUNTRACED`, retrying on `EINTR`. If the job was stopped, set `*stopped = 1` and return `128 + stop signal`. Otherwise set `*stopped = 0` and return the exit code, or `128 + signal` if it was killed. Return `-1` if `waitpid` fails.
