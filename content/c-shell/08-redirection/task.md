Implement `int run_redirect(char **argv)`.

`argv` is a `NULL`-terminated command, possibly containing the words `<`, `>`, `>>` and `2>`, each followed by a filename. Run the command in a child process with those redirections applied.

- The command line passed to the program must not contain the redirection operators or filenames (`echo hi > f` runs `echo hi`).
- `>` and `2>` create the file with mode `0666` and truncate it; `>>` creates and appends; `<` opens read-only.
- Each operator is processed left to right and a later one for the same stream replaces the earlier (earlier files are still created or truncated).
- If a file cannot be opened, print `sh: <file>: <strerror>` to stderr in the child, do not run the command, and return `1`.
- If an operator has no filename after it, run nothing and return `2`. If nothing is left after stripping, return `0`.
- Otherwise return the command's exit code (`127` if it cannot be started, `128 + signal` if it is killed).
- Do not change the parent's own descriptors, and leave no zombie. Do not modify the strings in `argv`.
