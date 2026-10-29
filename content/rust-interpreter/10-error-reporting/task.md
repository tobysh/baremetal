Implement `line_col`, `render_error` and `report`.

`line_col(src, pos)` returns one-based line and character column, counting `
` as a line break. Clamp offsets beyond source length to EOF; tests pass valid character boundaries.

For `Error::Runtime(r)`, return `runtime error: {r}`. For `Error::Parse(p)`, return exactly four lines, with no final newline:

```text
error: MESSAGE
  at line L, column C
L | SOURCE_LINE
  | SPACES^
```

The last line starts with `L.to_string().len()` spaces, then ` | `, then `C-1` spaces and `^`. An absent source line at EOF is empty. `report` returns successful output joined by `
`, otherwise `render_error`. Tests vary multiline/Unicode source, EOF and gutter widths.
