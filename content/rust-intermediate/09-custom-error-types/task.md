Parse a settings file such as:

```
# comment
width = 80
height = 24
```

`fn parse_config(text: &str) -> Result<Vec<(String, i64)>, ConfigError>` returns the `(key, value)` pairs in file order.

- Lines are trimmed. Blank lines and lines starting with `#` are skipped. Line numbers in errors start at 1 and count every line, including skipped ones.
- Key and value are split at the first `=` and trimmed.
- A line with no `=` is `MissingEquals(line)`.
- A value that is not an `i64` is `BadNumber { line, text }` where `text` is the trimmed value.
- A key that has already appeared is `Duplicate(key)`. Check the number first, then the duplicate.
- The first error found is the one returned.
- If there are no settings at all, the error is `NoSettings`.

Implement `Display` with exactly these messages:

| Variant | Message |
| --- | --- |
| `NoSettings` | `no settings found` |
| `MissingEquals(3)` | `line 3: expected key = value` |
| `BadNumber { line: 2, text: "x1" }` | `line 2: 'x1' is not a number` |
| `Duplicate("width")` | `duplicate key 'width'` |
