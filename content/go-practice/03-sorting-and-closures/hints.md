`SortPeople`: `sort.Slice(ps, func(i, j int) bool { ... })`. Compare ages first; only when they are equal compare names.
===
`TopWords`: count into a `map[string]int`, collect the keys into a slice, sort that slice with a function that compares `counts[a]` and `counts[b]` (higher first) and falls back to `a < b`. Then cut it to length `n` if it's longer.
===
`Counter`: declare `n := 0` before the `return func() int { ... }` and do `n++` inside it.
===
`Memoize`: create `cache := map[int]int{}` outside the returned function. Inside: look up with `v, ok := cache[x]`; on a miss, call `f(x)` and store the result.
