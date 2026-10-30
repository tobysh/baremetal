Write four functions:

- `fn middle(v: &[i32]) -> &[i32]` returns the slice without its first and last element. If `v` has two or fewer elements, return an empty slice.
- `fn last_word(s: &str) -> &str` returns the last whitespace-separated word, or `""` if there are no words. Extra spaces anywhere are ignored: `last_word("  a  bc  ")` is `"bc"`.
- `fn split_at_char(s: &str, c: char) -> (&str, &str)` splits at the first `c`. The first part is the text before it, the second is the text after it (not including `c`). If `c` does not occur, return `(s, "")`. It must work for non-ASCII `c`.
- `fn join_dash(parts: &[&str]) -> String` joins the parts with `-`: `["a", "b", "c"]` becomes `"a-b-c"`. No parts gives `""`.
