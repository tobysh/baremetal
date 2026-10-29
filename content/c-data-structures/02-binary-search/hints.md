Keep `lo = 0` and `hi = n`. While `lo < hi`, look at `mid = lo + (hi - lo) / 2` and either move `lo` past `mid` or pull `hi` down to `mid`.
===
For `lower_bound`: if `a[mid] < x` then `mid` is too small, so `lo = mid + 1`; otherwise `hi = mid`. Return `lo`.
===
`upper_bound` is identical except the test becomes `a[mid] <= x`. Then `count_equal` is `upper_bound(a, n, x) - lower_bound(a, n, x)`.
