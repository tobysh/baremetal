---
title: Strings, Bytes and Chars
summary: Understand UTF-8, the difference between bytes and characters, and the string methods that work safely on both.
minutes: 25
difficulty: medium
tags: strings, utf8, chars
id: rust-bridge/strings-and-chars
mode: harness
---
A C string is an array of bytes ending in `'\0'`, and `char` is one byte. A Rust `&str` is UTF-8 text: a slice of bytes that is guaranteed to be valid UTF-8, with a length but no terminator. One visible character can use 1 to 4 bytes.

```rust
let s = "héllo";
s.len()            // 6 bytes
s.chars().count()  // 5 characters
```

A Rust `char` is a Unicode scalar value (4 bytes), not a byte. Three views of the same text:

- `s.bytes()` yields `u8` values.
- `s.chars()` yields `char` values.
- `s.char_indices()` yields `(byte_offset, char)`, the right way to find positions.

## Why you cannot write s[2]

Indexing by integer is not allowed, because "the 2nd character" is not a quick lookup in UTF-8. You may slice by **byte range**, `&s[0..3]`, but only on character boundaries; slicing in the middle of a character panics. Test with `s.is_char_boundary(i)`.

## Handy methods

```rust
"  hi  ".trim()                 // "hi"
"a,b,,c".split(',')             // "a", "b", "", "c"
"one two".split_once(' ')       // Some(("one", "two"))
"Straße".to_uppercase()         // "STRASSE" (can change the length!)
'7'.to_digit(10)                // Some(7)
c.is_alphabetic(), c.is_ascii_digit(), c.is_alphanumeric()
s.starts_with("ab"), s.strip_prefix("ab")
```

Build strings with `String::new()`, `push(char)`, `push_str(&str)` or by collecting an iterator of chars: `chars.collect::<String>()`.

## This lesson's exercise

Four text utilities that must work for non-ASCII input too.
