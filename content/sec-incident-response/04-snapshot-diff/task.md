Implement `Diff(base, now []File, allowed map[string]string) []Change`.

`File` has `Path, Hash, Owner string`, `Mode uint32`, `MTime, CTime int64`. Return changes sorted by path, each `Change{Path, Kind string, Timestomp bool}`:

- `"added"`: path only in `now`. `"removed"`: path only in `base` (never timestomp).
- `"content"`: path in both, hash differs.
- `"meta"`: path in both, same hash, but mode or owner differs.
- Nothing is reported if only times differ.
- Allowlist: if `allowed[path] == now.Hash`, an `added` or `content` change is dropped. A dropped file whose mode or owner also differs from its baseline is still reported as `"meta"`.

`Timestomp` is true for reported added/content/meta entries when `MTime > CTime`, or when the kind is `"content"` and `MTime` equals the baseline's `MTime`.

Public example: a baseline file with hash `h1` and mtime 100 whose current hash is `h2`, mtime 100, ctime 900 is `content` with `Timestomp` true.
