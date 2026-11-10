Start with `nested` as the current best, then evaluate each remaining candidate in the tie order: hash, merge, then index (only when `IndexRight`). Compute each cost into a variable before comparing.
===
Replace the current best only when the new cost is strictly less (`<`). With `<=` a later candidate would win ties, which breaks the nested, hash, merge, index rule; the tests include exact ties for each pair.
===
For `merge`, start from `L+R` and add `4*L` only `if !s.SortedLeft`, and `4*R` only `if !s.SortedRight`. The two penalties are independent.
===
`Access` has nothing to do with the join: it is `"index"` exactly when `s.IndexLeft && 2+s.Selected < s.L`. If you see odd costs for a million rows, check that no value goes through plain `int`.
