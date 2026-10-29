`multiples_of`: `(1..).map(move |i| i * k)`. The `move` matters, because the closure outlives the function call.
===
`std::iter::successors(first, |&prev| next)` builds a sequence: it yields `first` (an `Option`), then keeps calling the closure, stopping at `None`. For `collatz`: `successors(if start == 0 { None } else { Some(start) }, |&n| if n == 1 { None } else if n % 2 == 0 { Some(n / 2) } else { Some(3 * n + 1) })`.
===
`digits_rev`: `let first = Some(n); successors(first, |&x| if x >= 10 { Some(x / 10) } else { None }).map(|x| x % 10)`. For `pick`, the branches have different types, so box each one: `"even" => Box::new((0..limit).filter(|n| n % 2 == 0))`, ... and `_ => Box::new(std::iter::empty())`.
