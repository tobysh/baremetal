Implement these string functions. The returned strings must borrow from the input, not be copied.

- `longest<'a>(a, b)`: the longer string, or `a` if the lengths are equal.
- `first_word(s)`: the first whitespace-separated word, or `""` if there is none (the string is empty or all whitespace).
- `split_pair<'a>(s, sep)`: split at the **first** `sep` character into `(before, after)` and return `Some`; `None` if `sep` does not appear. `sep` may be a multi-byte character.
- `strip_known<'a>(text, prefix)`: `text` with `prefix` removed from the front if it starts with it, otherwise `text` unchanged.
- `longest_line(text)`: the longest line of `text` by byte length, the first one on a tie, `""` for empty text.

Keep the lifetime annotations the starter gives you. They are what lets the tests use the results after the inputs are gone.
