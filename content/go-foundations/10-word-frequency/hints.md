Read the whole input with a `bufio.Scanner` line by line. For each line, `strings.ToLower` it, split with `strings.FieldsFunc` (separator: not a letter and not `'`), and increment a `map[string]int`.
===
Convert the map into a `[]WordCount`, then `sort.Slice` with the comparison: higher `Count` first, and if equal, smaller `Word` first.
===
Clamp before slicing: `if n > len(items) { n = len(items) }`, then `return items[:n]`. Handle `n <= 0` by returning `[]WordCount{}`. Imports: bufio, io, sort, strings, unicode.
