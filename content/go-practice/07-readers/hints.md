`ParseKV`: `sc := bufio.NewScanner(r)`, a counter `n` incremented at the top of each `sc.Scan()` loop. Trim the line; `continue` if it is empty or starts with `#`.
===
Then `k, v, ok := strings.Cut(line, "=")`. If `!ok` the error is `fmt.Errorf("line %d: missing '='", n)`. After trimming `k`, if it is empty the error is `empty key`. Store `m[k] = strings.TrimSpace(v)`.
===
`Stats`: read everything with `data, err := io.ReadAll(r)` (simple, and fine here). `bytes = len(data)`, `words = len(strings.Fields(string(data)))`.
===
Lines: count the `\n` characters with `strings.Count`, and add one if the data is non-empty and does not end with `\n`.
