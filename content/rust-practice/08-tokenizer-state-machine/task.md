`Token` and `LexError` are given in the starter (both derive `Debug`, `PartialEq`, `Clone`):

`Token`: `Num(i64)`, `Ident(String)`, `Plus`, `Minus`, `Star`, `Slash`, `LParen`, `RParen`.
`LexError`: `UnexpectedChar(char, usize)` and `BadNumber(usize)`. The `usize` is the **byte offset** of the character (or, for `BadNumber`, of the first digit).

Write `fn tokenize(input: &str) -> Result<Vec<Token>, LexError>`:

- Whitespace separates tokens and is otherwise ignored.
- A number is one or more ASCII digits, as `Num`. If it does not fit in an `i64`, return `BadNumber(start)`. There are no negative literals: `-5` is `Minus` then `Num(5)`.
- A name starts with a letter or `_` and continues with letters, digits or `_`; it becomes `Ident`. (Letters means `char::is_alphabetic`, so `café` is one name.)
- `+ - * / ( )` are single-character tokens.
- Any other character is `UnexpectedChar(c, offset)`; report the first one.
- Tokens that touch are still separate when their kinds differ: `12ab` is `Num(12)` then `Ident("ab")`, and `a1` is a single `Ident`.
- Empty or all-whitespace input gives an empty vector.

Use an explicit `State` enum, as in the lesson. The `State` type is yours to write.
