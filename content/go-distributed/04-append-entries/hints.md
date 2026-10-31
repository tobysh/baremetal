Work in this order: stale term; adopt term; prev check; merge; commit; reply. Early returns keep it readable.
===
Prev check: `PrevIndex > len(f.Log)` is a gap. Otherwise if `PrevIndex > 0`, compare `f.Log[PrevIndex-1].Term` with `PrevTerm`.
===
Merge loop: for each `i, e := range r.Entries`, `idx := r.PrevIndex + i` is the zero-based position. If `idx < len(f.Log)` and `f.Log[idx].Term == e.Term`, continue. If `idx < len(f.Log)` the terms differ, so `f.Log = f.Log[:idx]`. Then `f.Log = append(f.Log, e)`.
===
After a conflict you have truncated, so later entries are all appended: the "same term, skip" case cannot happen again. Do not truncate when everything matches; a shorter duplicate request must leave a longer log alone.
===
Commit: `last := r.PrevIndex + len(r.Entries)`; if `r.LeaderCommit > f.Commit`, `f.Commit = min(r.LeaderCommit, last)`. If `last` is below the current commit, do not lower it (the `>` check on the `min` result handles that).
