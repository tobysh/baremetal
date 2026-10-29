Implement two functions.

`int find_in_path(const char *name, const char *path, char *out, size_t out_size)`

- If `name` contains a `/`, accept it as it is: copy it to `out` and return `0` if it is a regular file you may execute, otherwise `-1`.
- Otherwise try each directory of the colon-separated `path` in order (skip empty entries). The first `dir/name` that is a regular, executable file is written to `out`; return `0`.
- Return `-1` if nothing matches or if the result does not fit in `out_size` bytes (terminator included). Do not modify `path`.

`int run_with_path(char *const argv[], const char *path)` runs `argv[0]` in a child, found with `find_in_path` and started with `execv` (not `execvp`). Return its exit code. Return `127` if the command is not found, `-1` if `fork` fails or the child is killed by a signal.
