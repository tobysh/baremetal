For `swap`, a reference is written through with `*`. Save one value in a temporary: `let t = *a; *a = *b; *b = t;`.
===
`double_all`: `for x in v.iter_mut() { *x *= 2; }`. `iter_mut` gives `&mut i32` items, so each needs a star to change it. For `count_matching`, iterate `words.iter()` and compare each `&String` with `target` using `w == target`, or use `.filter(...).count()`.
===
For `largest_mut`, `v.iter_mut().max_by_key(|x| **x)` already returns `Option<&mut i32>`. The closure receives `&&mut i32`, hence the two stars.
