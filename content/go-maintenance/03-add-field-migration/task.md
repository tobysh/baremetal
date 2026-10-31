Extend the member book. Keep every existing behaviour, and:

1. `Member` has a `Phone string` field (already declared in the starter). Make it real: read it, store it, write it.
2. `Load` reads v1 and v2 lines mixed in one file. v1 members get `Phone == ""`. Blank lines and lines starting `#` are skipped, `\r\n` line endings work, as before.
3. `Save` writes **every** member as a v2 line, in insertion order, one per line, with a trailing newline (empty book: empty string).
4. v2 escaping as in the lesson: `\\`, `\|`, `\n`, `\r`. Any other escape or a trailing lone backslash is an error. v2 fields are kept exactly (no trimming). A v2 line has exactly five fields.
5. A line starting `v` plus digits and `|` other than `v2|` (for example `v3|...`) is an error: `line N: unsupported version "v3"`. `Load` returns `nil` and the error; no partial book.
6. `Add` now allows `|` and line breaks in any field (they are escaped on save). The name must still be non-empty, ids must be positive, and a duplicate id is still an error.
7. New `Migrate(data string) (string, error)`: convert a whole file to v2 line by line, keeping blank and comment lines **exactly** as they are and every record line in its original position, with `\n` line endings. Re-running `Migrate` on its own output gives the same output. On a bad line return `"", err` with `line N:` in the message.

Existing error text for v1 problems (`line N: ...`) keeps its `line N:` prefix.
