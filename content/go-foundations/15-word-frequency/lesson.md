---
title: Practical Go - a word counter
summary: Put it together: read text from an io.Reader with bufio, count and sort words, and write code that is easy to test.
minutes: 25
difficulty: hard
tags: maps, strings, io
id: go-foundations/word-frequency
mode: harness
---
Real Go programs are small functions that take and return plain values and talk to the outside world through **interfaces**. The best-known are `io.Reader` and `io.Writer`: a file, a network connection, an HTTP body and a `strings.Reader` all satisfy them. A function that takes an `io.Reader` can be tested with a string and used with a file, without changing a line.

## Reading lines and words

`bufio.Scanner` reads a stream piece by piece:

```go
sc := bufio.NewScanner(r)
for sc.Scan() {               // one line at a time
	line := sc.Text()
	...
}
if err := sc.Err(); err != nil { ... }

sc.Split(bufio.ScanWords)     // or: split on whitespace instead of newlines
```

## Cleaning words

`unicode.IsLetter(r)` tests one rune. `strings.FieldsFunc(s, f)` splits `s` wherever `f(rune)` returns true, handy for splitting on anything that is not a letter or apostrophe:

```go
words := strings.FieldsFunc(line, func(r rune) bool {
	return !unicode.IsLetter(r) && r != '\''
})
```

## Sorting

`sort.Slice` sorts any slice with a comparison function. When counts tie, break the tie so the output is deterministic (maps iterate in random order, so you must sort):

```go
sort.Slice(items, func(i, j int) bool {
	if items[i].Count != items[j].Count {
		return items[i].Count > items[j].Count   // bigger first
	}
	return items[i].Word < items[j].Word          // then alphabetical
})
```

## Slicing the top N

`items[:n]` panics when `n > len(items)`, so clamp first. A tiny helper like `min` (a builtin since Go 1.21) keeps that tidy.

## Putting it together

A good Go program reads input, transforms it with pure functions, and prints. Here you build the middle: counting and ranking. Wrapping it in a `main` that opens `os.Stdin` is a one-liner: `TopWords(os.Stdin, 10)`.
