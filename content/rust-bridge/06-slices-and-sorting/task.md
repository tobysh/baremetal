Complete these in the starter:

- `fn median(v: &[i32]) -> Option<f64>`: the middle value, or the mean of the two middle values when the length is even. `None` for an empty slice. The input must not be modified (copy it first).
- `fn top_n(v: &[i32], n: usize) -> Vec<i32>`: the `n` largest values in descending order (duplicates count separately). Fewer if there are fewer than `n`.
- `fn insert_sorted(v: &mut Vec<i32>, x: i32)`: insert `x` into the already sorted vector so it stays sorted. Use `binary_search`. Equal values may go on either side of each other.
- `fn unique_sorted(v: &[i32]) -> Vec<i32>`: the distinct values in ascending order.
- `fn ends(v: &[i32]) -> Option<(i32, i32)>`: the first and last item with a slice pattern. `None` when empty; a one-item slice gives that item twice.
- `fn sort_people(people: &mut Vec<(String, u32)>)`: sort by age (the `u32`) descending, and by name ascending when ages are equal.
