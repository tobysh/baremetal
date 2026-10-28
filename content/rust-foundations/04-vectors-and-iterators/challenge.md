**Challenge: the median.**

Write `fn median(v: &[i32]) -> Option<f64>`:

- an empty slice gives `None`;
- an odd number of values gives the middle value once they are sorted;
- an even number gives the average of the two middle values (so the answer can end in `.5`).

The input is borrowed, so it must **not** be changed or reordered. Copy it into a `Vec` first (`v.to_vec()`), sort the copy, then pick from it. Take care not to overflow when you add two large `i32`s: do the arithmetic in `f64` (or `i64`).
