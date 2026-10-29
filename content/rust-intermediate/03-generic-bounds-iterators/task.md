Complete these in the starter.

- `Countdown { n }` implements `Iterator<Item = u32>` and yields `n, n-1, ..., 1`, then stops. `Countdown { n: 0 }` yields nothing.
- `fn largest_by_key<T, K, F>(items: &[T], key: F) -> Option<&T>`: the item whose key is greatest, the **first** one on a tie, `None` for an empty slice.
- `fn summarize<I>(it: I) -> String`: the items printed with `Display` and joined by `", "`. Nothing gives an empty string.
- `fn every_other<I: Iterator>(it: I) -> Vec<I::Item>`: the items at positions 0, 2, 4, and so on.

Tests use different item types (numbers, strings, floats, chars) and your own `Countdown`.
