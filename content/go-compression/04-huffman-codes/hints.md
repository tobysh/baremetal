Make a node per used symbol with `weight` and `minSym`. Keep a list of active nodes; each round pick the two smallest by `(weight, minSym)`.
===
The merged node has weight `a.w+b.w` and `minSym = min(a.minSym, b.minSym)`. Remember children so that you can walk the final tree and record depths.
===
With 256 symbols an O(n^2) selection is fine.
===
Canonical: count symbols per length. `code = (code + count[l-1]) << 1` gives the first code of length `l`. Then walk the symbols in order and hand out `next[l]++`.
===
Oversubscribed check: add `1 << (48-l)` for every used symbol and fail when the sum passes `1<<48`.
