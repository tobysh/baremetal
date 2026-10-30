Write four functions. The parameter types are fixed in the starter; you fill in the bodies.

- `fn swap(a: &mut i32, b: &mut i32)` exchanges the two values.
- `fn double_all(v: &mut Vec<i32>)` multiplies every element by 2, in place.
- `fn count_matching(words: &Vec<String>, target: &str) -> usize` counts how many words equal `target` exactly (case matters). It only reads.
- `fn largest_mut(v: &mut Vec<i32>) -> Option<&mut i32>` returns a mutable reference to the largest element, or `None` if the vector is empty. The caller will write through it. Values in the tests are distinct.

Do not clone the vectors.
