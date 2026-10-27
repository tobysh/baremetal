In the starter, implement:

- `fn longest<'a>(a: &'a str, b: &'a str) -> &'a str` returns the longer string. If they are equal in length, return `a`.
- `fn first_word(s: &str) -> &str` returns everything before the first space, or the whole string if there is no space. Leading spaces are not expected.
- `struct Words<'a> { rest: &'a str }` with `fn new(text: &'a str) -> Words<'a>` (already written) and `fn next_word(&mut self) -> Option<&'a str>`. It skips leading spaces, returns the next word and advances past it. It returns `None` when only spaces (or nothing) remain.

Do not allocate new `String`s; return slices of the input.
