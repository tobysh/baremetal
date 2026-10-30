---
title: Strings, runes and bytes
summary: A Go string is read-only bytes of UTF-8 text. Learn the difference between a byte and a rune, why len is not a character count, and how to convert between them.
minutes: 22
difficulty: medium
tags: strings, runes, unicode
id: go-foundations/strings-runes-bytes
mode: harness
---
A Go `string` is an immutable sequence of **bytes**, and by convention those bytes are **UTF-8** text. A `byte` is a `uint8`. A `rune` is an `int32` holding one Unicode code point, which UTF-8 stores in one to four bytes.

```go
s := "héllo, 世界"
fmt.Println(len(s))                    // 14: bytes, not characters
fmt.Println(utf8.RuneCountInString(s)) // 9: characters (runes)
fmt.Println(s[1])                      // 195: indexing gives a byte
```

`é` takes two bytes and each of `世` and `界` takes three, so `len` and indexing count bytes. That is fine for ASCII and wrong for most of the world's text.

## range decodes for you

```go
for i, r := range "héy" {
	fmt.Println(i, string(r)) // 0 h, 1 é, 3 y
}
```

`range` over a string steps one **rune** at a time. `i` is the byte offset where the rune starts (so it jumps from 1 to 3 here) and `r` is a `rune`. Invalid UTF-8 shows up as `utf8.RuneError` (`U+FFFD`).

## Converting

```go
runes := []rune(s)      // decode into a slice of code points
bytes := []byte(s)      // copy the raw bytes
back  := string(runes)  // encode runes back into a string
```

Each conversion **copies**, because strings are immutable. `string(r)` of a single rune makes a one-character string (`string(rune(65))` is `"A"`; do not convert an `int` straight to `string`). Slicing a string such as `s[:3]` cuts at **byte** positions and can split a character in half, so to cut by characters, convert to `[]rune` first.

## Useful helpers

- `unicode.IsLetter(r)`, `unicode.IsUpper(r)`, `unicode.ToLower(r)` work on single runes.
- `strings.Builder` builds a string without quadratic copying: `var b strings.Builder; b.WriteRune(r); b.String()`.
- Rune literals use single quotes: `'a'` is a rune, `"a"` is a string. Arithmetic such as `'a' + 2` works on runes.
