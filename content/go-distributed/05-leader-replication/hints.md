Keep `next` and `match` as `[]int` of length `N`. Index them by peer number and just never use `l.ID`.
===
`Request`: `prev := l.next[peer]-1`; `PrevTerm` is `l.Log[prev-1].Term` when `prev > 0`. Slice `l.Log[prev:end]` and copy it with `append([]Entry(nil), ...)`.
===
`OnReply` failure path: `n := resp.Match + 1`; `if n < l.next[peer] { l.next[peer] = n }`; then `if l.next[peer] < l.match[peer]+1 { l.next[peer] = l.match[peer]+1 }`. This makes stale replies harmless.
===
Commit: loop `i` from `len(l.Log)` down to `l.Commit+1`. Skip entries whose term is not `l.Term`. Count `1` for yourself plus peers with `match[p] >= i`. The first `i` with a majority is the new commit; stop.
===
Call the commit step from `Propose` (for the one-node cluster) as well as from `OnReply`.
