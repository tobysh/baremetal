Complete these in the starter. `BTreeMap` and `BTreeSet` are imported:

- `fn index_words(lines: &[&str]) -> BTreeMap<String, Vec<usize>>`: map each lowercase word to the 1-based line numbers where it appears, ascending, with no repeats within a line (a word appearing twice in one line lists that line once).
- `fn alphabetical(index: &BTreeMap<String, Vec<usize>>) -> Vec<String>`: the words that appear on more than one line, in map order.
- `fn between(prices: &BTreeMap<u32, String>, lo: u32, hi: u32) -> Vec<String>`: the names whose price is `lo <= price <= hi`, in price order. If `lo > hi` return an empty vector (careful: `range` panics for a reversed range).
- `fn cheapest_at_least(prices: &BTreeMap<u32, String>, min: u32) -> Option<&String>`: the name with the smallest price that is at least `min`.
- `fn priciest_below(prices: &BTreeMap<u32, String>, limit: u32) -> Option<&String>`: the name with the largest price strictly below `limit`.
- `fn distinct_gaps(v: &[i32]) -> BTreeSet<i32>`: the set of differences between consecutive values after sorting and de-duplicating `v`. Fewer than two distinct values gives an empty set.
