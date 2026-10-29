Complete these in the starter:

- `fn run_lengths(s: &str) -> Vec<(char, usize)>`: group consecutive equal characters, so `"aaabcc"` gives `[('a', 3), ('b', 1), ('c', 2)]`. Use `peekable`. An empty string gives an empty vector.
- `fn all_words(lines: &[&str]) -> Vec<String>`: split every line on whitespace and return all words in lowercase, in order. Use `flat_map`.
- `fn split_by_sign(v: &[i32]) -> (Vec<i32>, Vec<i32>)`: the first vector holds the values that are zero or greater, the second the negative ones; keep their order. Use `partition`.
- `fn first_big_square(v: &[i32], limit: i32) -> Option<i32>`: the square of the first number whose square is greater than `limit`. Use `find_map`. Use `i64` internally if you like, but the squares in the tests fit in `i32`.
- `fn numbers_in(items: &[&str]) -> Vec<i32>`: parse each item as an `i32` and keep only those that parse (use `flatten` or `filter_map`). `"12"` and `"-4"` parse; `"x1"` and `""` do not.
