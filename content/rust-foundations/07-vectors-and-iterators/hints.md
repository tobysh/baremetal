Both can be written with iterators: `v.iter()` walks the slice. For `evens_squared`, you need a `filter` to keep evens and a `map` to square.
===
`v.iter().filter(|&&x| x % 2 == 0).map(|&x| x * x).collect()`. For `average`, first handle the empty case by returning `None`.
===
For `average`: `if v.is_empty() { None } else { Some(v.iter().sum::<f64>() / v.len() as f64) }`. The `::<f64>` tells `sum` what type to add up.
