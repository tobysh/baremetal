Complete these in the starter. The tests include accented letters and emoji, so do not index bytes by hand.

- `fn is_palindrome(s: &str) -> bool`: ignore everything that is not alphanumeric and ignore case; `"A man, a plan, a canal: Panama"` is a palindrome. An empty or punctuation-only string counts as one.
- `fn title_case(s: &str) -> String`: split on whitespace, capitalise the first letter of each word and lowercase the rest, join with single spaces. `"  hELLO   wORLD "` becomes `"Hello World"`.
- `fn truncate_chars(s: &str, max: usize) -> String`: at most `max` characters of `s` (not bytes), and never cut a character in half.
- `fn caesar(s: &str, shift: i32) -> String`: shift ASCII letters by `shift` places in the alphabet, wrapping around, keeping case. Everything else is unchanged. The shift may be negative or larger than 26.
- `fn split_key_value(line: &str) -> Option<(&str, &str)>`: split `key=value` at the first `=`, trimming spaces around both parts. `None` if there is no `=` or the key is empty after trimming. The value may be empty or contain more `=` signs.
