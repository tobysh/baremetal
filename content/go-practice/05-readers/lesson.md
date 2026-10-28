---
title: Reading Text from an io.Reader
summary: Parse input from any source with io.Reader and bufio.Scanner, and report errors with line numbers.
minutes: 30
difficulty: medium
tags: io, parsing
id: go-practice/readers
mode: harness
---
In Go, "something you can read bytes from" is the interface `io.Reader`. Files, network connections, HTTP bodies, and `strings.NewReader("text")` all satisfy it. Write your function to take an `io.Reader` and it works with all of them, and is trivial to test: pass it a string.

## Reading line by line

`bufio.Scanner` splits input into lines (or words):

```go
sc := bufio.NewScanner(r)
for sc.Scan() {
	line := sc.Text() // without the trailing newline
	...
}
if err := sc.Err(); err != nil { // nil unless a read failed
	return err
}
```

`Scan` returns `false` at the end of input *or* on an error, so always check `sc.Err()` afterwards. Switch to word mode with `sc.Split(bufio.ScanWords)` before the first `Scan`.

## Useful string helpers

- `strings.TrimSpace(s)` removes leading and trailing whitespace.
- `strings.Cut(s, "=")` splits at the first `=` and returns `before, after, found`.
- `strings.HasPrefix(s, "#")` tests how a string starts.

```go
key, value, ok := strings.Cut("name = Ada", "=")
// "name ", " Ada", true: trim both before using them
```

## Errors with a position

A parse error that says "bad input" is useless in a 500-line file. Say *where*: `fmt.Errorf("line %d: missing '='", n)`. Keep a counter that starts at 1 and counts every line, including blank ones.
