Write `fn word_count(text: &str) -> HashMap<String, usize>` that counts how many times each word appears.

- Split `text` on whitespace.
- Lowercase each word.
- Strip leading and trailing ASCII punctuation with `trim_matches(|c: char| c.is_ascii_punctuation())`.
- Skip any word that becomes empty after stripping (such as `"--"`).

For `"The cat; the DOG. the end!"` the result has `the` -> 3, `cat` -> 1, `dog` -> 1, `end` -> 1.
