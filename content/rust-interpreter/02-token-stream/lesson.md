---
title: "A Peekable Token Stream"
summary: Give the parser a cursor with lookahead, conditional consumption and precise expectation errors.
minutes: 30
difficulty: easy
tags: parser, lookahead, errors
id: rust-interpreter/token-stream
mode: harness
---
Builds on [Lexing with Source Positions](/learn/rust-interpreter/lexer).

The lexer returns a vector, but a parser should not repeat indexing and end-of-input handling everywhere. `Stream` wraps that vector with a cursor `i` and the original source length `end`.

Separate **looking** from **consuming**. `peek()` borrows the next token and leaves `i` unchanged. `peek_at(n)` looks `n` places further ahead. `next()` clones the next `Spanned` and moves the cursor only when there was a token. Calling it repeatedly at EOF must stay safe.

`eat(token)` is useful for optional punctuation: consume exactly one matching token and return true; a mismatch returns false without changing the stream. `expect(token)` handles required punctuation. Its successful result is the consumed token's offset, useful for future syntax nodes. A failure names what was expected, what was found, and the current source offset.

For example, expecting `Semi` before `RParen` reports `expected Semi, found RParen`. At EOF, the same operation reports `expected Semi, found end of input`. The error position is the start of the unexpected token, or `end` when there is none. Trailing whitespace therefore contributes to EOF positions even though it produces no tokens.

`expect_ident()` follows the same contract but extracts a name from `Ident(String)`. Do not consume a mismatching token: callers may need it to diagnose or recover. `at_end()` checks the cursor against the vector length.

This layer has no idea what expressions mean. It establishes one reliable set of cursor rules so the recursive parser can focus on grammar.
