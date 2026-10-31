`counter_new`: `Box::into_raw(Box::new(Counter::new(step)))`.
===
`counter_bump`: `if c.is_null() { return 0 } let c = &mut *c; c.total = c.total.wrapping_add(c.step as u64); c.total`.
===
`counter_free`: `if !c.is_null() { drop(Box::from_raw(c)); }`. `counter_into_inner`: read the total first, then free through `counter_free`.
