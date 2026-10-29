Complete these in the starter. `HashMap` and `HashSet` are already imported:

- `fn first_duplicate(v: &[i32]) -> Option<i32>`: the first value that has already appeared earlier in the slice, scanning left to right. Use a `HashSet` and the `bool` returned by `insert`.
- `fn word_counts(text: &str) -> HashMap<String, usize>`: count words (split on whitespace, compare in lowercase). Use `entry`.
- `fn group_by_first_letter(words: &[&str]) -> HashMap<char, Vec<String>>`: group words by their first character, lowercased, keeping the words as given and in input order. Skip empty words. Use `entry(..).or_default()`.
- `fn common_sorted(a: &[i32], b: &[i32]) -> Vec<i32>`: the values present in both slices, each once, in ascending order.
- `fn only_in_first_sorted(a: &[i32], b: &[i32]) -> Vec<i32>`: the values in `a` that are not in `b`, each once, ascending.
- `fn most_common(text: &str) -> Option<(String, usize)>`: the most frequent lowercase word and its count; on a tie, the alphabetically smallest word. `None` if there are no words. You may reuse `word_counts`.
