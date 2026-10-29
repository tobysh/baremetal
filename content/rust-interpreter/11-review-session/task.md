Implement `Session::new`, `ok_lines`, `eval_line` and `transcript`.

Keep one fresh `Env` per session and an initially zero success count. Skip whitespace-only inputs. Try `parse_expression` first: a successfully parsed expression is evaluated and echoed. Only a parse failure falls back to `exec_source`.

After either path, drain `env.take_output()`. On expression success append the displayed value; on failure append `render_error`. Successful nonblank attempts increment the count once. Join response lines with `
` and no trailing newline. State survives between inputs; partial runtime mutations are retained.

`transcript` records `> LINE` for every input, including blanks, and adds its response only when nonempty, joining everything with `
`. Tests use different sessions, closures, partial failures, prints and counters.
