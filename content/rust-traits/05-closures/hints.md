`make_adder` returns a closure. Because it outlives the function, use `move`: `move |x| x + n`.
===
`apply_n`: keep a running value, `let mut value = x;`, then `for _ in 0..times { value = f(value); }` and return `value`.
===
`make_counter`: declare `let mut count = 0;` inside the function, then return `move || { count += 1; count }`. The closure owns its own `count`, so two counters never interfere.
