---
title: A Tokenizer as a State Machine
summary: Turn text into tokens with a hand-written state machine, using an enum for the state, tuple matches with guards, and a custom error with positions.
minutes: 30
difficulty: medium
id: rust-practice/tokenizer-state-machine
mode: harness
---
Before a program can understand `12 + count * (3 - x)`, something has to split it into meaningful chunks: the number `12`, the symbol `+`, the name `count`, and so on. That step is called **tokenizing** (or lexing), and its output is a list of **tokens**.

```rust
enum Token {
    Num(i64),
    Ident(String),
    Plus,
    LParen,
    // ...
}
```

## A state machine

Reading character by character, you are always in one of a few situations: between tokens, in the middle of a number, in the middle of a name. Make that explicit with an enum, and the code becomes a table of "in this state, seeing this character, do that".

```rust
enum State {
    Start,
    Number { start: usize, text: String },
    Ident(String),
}
```

The loop looks at the current character together with the current state and decides what to do:

```rust
state = match (state, current) {
    (State::Start, Some((_, c))) if c.is_ascii_digit() => { /* begin a number */ }
    (State::Number { start, mut text }, Some((_, c))) if c.is_ascii_digit() => { /* extend it */ }
    (State::Number { start, text }, _) => { /* a non-digit or the end: finish the token */ }
    // ...
};
```

Matching a **tuple** of the state and the input, with guards, is a very Rust way to write this. Notice that finishing a number does **not** consume the character that ended it: you go back to `Start` and look at the same character again. That is why the code tracks an index `i` and only advances it in some arms.

Matching on `state` by value moves it, so each arm must produce the next state. The compiler will insist that every combination is covered, which catches missed cases for you.

## Errors with a position

When the input contains something the tokenizer does not understand, return an error that says what and where, so a human can find it:

```rust
enum LexError {
    UnexpectedChar(char, usize), // the character and its byte offset
    BadNumber(usize),            // too large for an i64
}
```

`char_indices()` gives each character together with its byte offset.

## This lesson's exercise

Write `tokenize` with a state machine. The `Token` and `LexError` types are given.
