Collect the input first: `let chars: Vec<(usize, char)> = input.char_indices().collect();` and keep an index `i`. Then `let cur = chars.get(i).copied();` is `None` at the end of the input, so the end is just another case your `match` handles.
===
Define `enum State { Start, Number { start: usize, text: String }, Ident(String) }` and loop with `state = match (state, cur) { ... }`. In `Start`, a space advances `i` and stays in `Start`; a digit or letter starts the matching state; a symbol pushes a token; anything else is the error.
===
The arm `(State::Number { start, text }, _)` (no guard, placed after the digit-extending arm) means "the number is over". Parse `text` with `text.parse::<i64>()`, convert a failure with `map_err(|_| LexError::BadNumber(start))?`, push `Token::Num(n)`, and return `State::Start` without advancing `i`.
===
`(State::Start, None) => break` ends the loop, but only after the other states have had their chance to finish a pending token at the end of the input, which is why `Number` and `Ident` finishing arms use `_` for the input.
