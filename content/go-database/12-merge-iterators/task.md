`Entry`, the `Iterator` interface, `NewSliceIter(entries []Entry) Iterator` (sorted entries) and `Collect(it Iterator) []Entry` are given. Implement three wrappers. Each must be lazy: take entries from the sources only when `Next` is called.

- `Merge(sources ...Iterator) Iterator`: each source yields entries in ascending key order with unique keys, and `sources[0]` is the newest. The merged iterator yields keys in ascending order, once each. For a key that several sources hold, it returns the entry from the lowest-numbered source (tombstones included) and consumes that key from all the others.
- `Live(it Iterator) Iterator` skips entries with `Delete` set.
- `Range(it Iterator, start, end string) Iterator` skips keys `< start`, and ends (permanently, without reading further) at the first key `>= end`. An empty `end` means unbounded; an empty `start` means from the beginning.

Once an iterator has returned `false` it keeps returning `false`.
