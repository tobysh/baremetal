Put the shared argument check in a helper: `unsafe fn key<'a>(s: *const Store, k: *const u8, n: usize) -> Result<&'a [u8], i32>`. Order: null store, then `n == 0` (BAD_KEY), then null key.
===
`kv_add`: `let cur = store.map.get(k).copied().unwrap_or(0); match cur.checked_add(delta) { None => OVERFLOW, Some(v) => { store.map.insert(k.to_vec(), v); if !out.is_null() { *out = v; } OK } }`.
===
`kv_each`: build `Entry { key: k.as_ptr(), key_len: k.len(), value: v }` for each `(k, v)` of the `BTreeMap` (already sorted by bytes) and count calls; `BTreeMap<Vec<u8>, _>` orders byte strings lexicographically.
