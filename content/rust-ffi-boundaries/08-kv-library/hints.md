Four of the six functions start with the same argument checks, so write them once in a private helper that returns a safe `&[u8]` or an error code. Fix the check order (null store, empty key, null key) and use it everywhere.
===
`kv_add`: read the current value (a missing key counts as 0), use `checked_add`, and touch the map only when the addition succeeded. That is what makes an overflow leave no trace, including no newly created key.
===
`kv_each`: the `BTreeMap` already iterates in ascending byte order. Build an `Entry` on the stack for each pair, pass its address to the callback, count the calls and stop when the callback returns non-zero.
===
`kv_new` and `kv_free` are lesson 5 again: create with `Store::new`, box, convert to raw; free with null as a no-op. If the random-model test fails, the failing message names the seed and step, so replay that sequence by hand.
