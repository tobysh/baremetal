**Challenge: the longest common prefix.**

Write `fn common_prefix<'a>(words: &[&'a str]) -> &'a str` that returns the longest prefix shared by every word, as a slice of the first word. Nothing is copied.

- No words gives `""`. One word gives that whole word.
- Compare whole characters, not bytes. `"naïve"` and `"naïf"` share `"naï"`, and you must never cut a multi-byte character in half or the program panics.
- The match is case-sensitive: `"Cat"` and `"cat"` share nothing.
- The result must borrow from the first word (the signature's `'a` says so), so slice it with `&first[..end]` rather than building a `String`.
- Hint: `char_indices()` gives byte positions, and `zip` lets you walk two words together.
