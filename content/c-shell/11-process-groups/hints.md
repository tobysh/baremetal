`spawn_group`: in the child call `setpgid(0, 0)` before `execvp`. In the parent call `setpgid(pid, pid)` right after `fork` and ignore its result.
===
`kill_group(pgid, sig)` is `kill(-pgid, sig)`. A negative first argument means "the group with this id".
===
`wait_job`: `waitpid(pid, &status, WUNTRACED)` in a loop on `EINTR`; check `WIFSTOPPED(status)` first (then `WSTOPSIG`), then `WIFEXITED`, then `WIFSIGNALED`. A stopped job has not ended, so it has not been reaped; it can be waited for again after SIGCONT.
