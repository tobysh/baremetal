---
title: "Lexing with Source Positions"
summary: Turn source text into tokens while retaining byte offsets for useful diagnostics.
minutes: 35
difficulty: medium
tags: lexer, tokens, source-positions
id: rust-interpreter/lexer
mode: harness
---
Before a program can run, its text needs structure. This course builds one small language in Rust, then gives it a second execution engine. Each exercise includes all the earlier working pieces; implement only the functions marked `todo!()`. You can solve any lesson without copying files from another editor.

A **lexer** turns characters into tokens. `let total = 12 + 3;` becomes `Let`, `Ident("total")`, `Eq`, `Num(12)`, `Plus`, `Num(3)`, `Semi`. Punctuation and keywords already have variants in `Token`; identifiers and integers carry their values.

`Spanned` pairs a token with its starting **byte offset**. Rust strings use UTF-8: a character may take several bytes. Iterating with `char_indices()` supplies both the offset and the character, so positions remain correct even after non-ASCII text in comments. Identifiers in this language are deliberately ASCII; whitespace and comment contents may be Unicode.

Scan left to right. Skip whitespace. A `#` begins a comment lasting until newline or end of input. On a digit, gather the complete run of digits and parse it as an `i64`. Overflow becomes `NumberTooBig` at the first digit. A minus is its own token; negative numbers are parsed as unary negation later.

Names start with an ASCII letter or underscore, then accept ASCII letters, digits and underscores. Match the complete word against the nine keywords. `if` is a keyword; `iffy` is a name.

Operators use the longest available match: `>=` is one token, while `> =` is two. Look ahead before consuming `=`, `!`, `<` or `>`. Return the first lexical error rather than skipping an unfamiliar character. Empty source is valid and produces no tokens.

The lexer recognizes the whole language now, although evaluation grows over later lessons. Keeping token recognition independent of parsing makes both layers easier to test.
