Complete these in the starter:

- `fn map_reduce<T, U, M, R>(items: &[T], workers: usize, map: M, identity: U, reduce: R) -> U` with the bounds shown there. Split `items` into chunks of size `ceil(len / workers)` (at least 1; `workers == 0` counts as 1). Each scoped thread folds its chunk: start from `identity.clone()`, and for each item do `acc = reduce(acc, map(item))`. Then fold the per-chunk results **in chunk order**, starting from `identity`. An empty slice returns `identity`.
- `fn word_frequencies(texts: &[&str], workers: usize) -> BTreeMap<String, usize>`: count lowercase words (split on whitespace) over all the texts using `map_reduce`.
