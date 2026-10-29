Implement `lex(src)` using the supplied `Token`, `Spanned` and `LexError` types.

- Skip Unicode whitespace and `#` comments through the next newline or EOF.
- Recognize decimal nonnegative `i64` literals; report `NumberTooBig(start)` on overflow.
- Names match `[A-Za-z_][A-Za-z0-9_]*`. Recognize `let if else while fn return print true false` as keywords.
- Recognize every punctuation/operator variant shown in `Token`, including `== != <= >=` before their single-character forms.
- Record each token's starting byte offset. Return `UnexpectedChar(character, offset)` at the first unsupported character.
- Empty input produces `Ok(vec![])`.

The hidden tests vary text, whitespace, comments, operator adjacency, integer boundaries and UTF-8 offsets.
