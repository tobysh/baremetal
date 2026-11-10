Ownership of the heap value must leave Rust when the handle is created and come back exactly once when it is destroyed. Look at what `Box` offers for turning an owned value into a raw pointer and back.
===
`counter_new`: create the counter with `Counter::new` (the tests count those), put it in a `Box`, and convert the box into a raw pointer. `counter_bump` and `counter_total` borrow through the pointer; they must check for null first and must never free.
===
`counter_free` is the only function that rebuilds a `Box` from the pointer, and dropping that box runs `Drop`. A null handle should do nothing. Note that bump and total use `wrapping_add` semantics for the step.
===
`counter_into_inner`: read the total before the memory goes away, then release the handle with the function you already wrote. If the final `live()` assertion fails, count your `from_raw` calls against your `into_raw` calls.
