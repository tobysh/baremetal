`randomHeight`: `h := 1; for h < maxHeight && s.rng.Intn(2) == 0 { h++ }; return h`.
===
Write one helper that fills `update [maxHeight]*node` using the loop from the lesson and returns the candidate `update[0].next[0]`. `Get`, `Put`, `Delete` and `Seek` all start from it.
===
`Put` on a new key: `h := s.randomHeight()`. If `h > s.height`, set `update[lvl] = s.head` for the levels from `s.height` up to `h-1`, then `s.height = h`. Create `n := &node{key: key, value: v, next: make([]*node, h)}` and for each `lvl < h` do `n.next[lvl] = update[lvl].next[lvl]; update[lvl].next[lvl] = n`.
===
`Delete`: if the candidate's key is not `key`, return false. Otherwise for each `lvl` while `update[lvl].next[lvl] == cand` set `update[lvl].next[lvl] = cand.next[lvl]`. Then `for s.height > 1 && s.head.next[s.height-1] == nil { s.height-- }`.
===
`Seek` is just the helper's candidate: if it is nil return `"", nil, false`. `Keys` follows `head.next[0]`, then `.next[0]`, until nil.
