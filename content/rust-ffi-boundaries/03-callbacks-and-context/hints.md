`for_each`: after the null checks, `for i in 0..len { let v = *xs.add(i); if cb(ctx, v) != 0 { return (i + 1) as isize; } } len as isize`.
===
Inside a callback: `let s = unsafe { &mut *(ctx as *mut Stats) };`. For `max`: `s.max = Some(s.max.map_or(v, |m| m.max(v)))`.
===
`stats_of`: `let mut s = Stats { count: 0, sum: 0, max: None }; unsafe { for_each(xs.as_ptr(), xs.len(), collect_stats, &mut s as *mut Stats as *mut c_void) }; s`. A slice's `as_ptr()` is non-null even when empty, which is fine because `len` is 0.
