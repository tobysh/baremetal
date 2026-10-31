Implement `Follower.HandleAppend` in `starter.go`. `Follower.Log[i]` holds the entry at index `i+1`.

1. A request with `Term` lower than the follower's is rejected: `AppendResp{Term: f.Term, Success: false, Match: 0}` and no state changes.
2. Otherwise adopt the request's term.
3. If `PrevIndex > 0` and the follower has no entry at `PrevIndex` or its term is not `PrevTerm`, reject with `Match = min(len(Log), PrevIndex-1)`; the log is unchanged.
4. Otherwise merge `Entries` starting at index `PrevIndex+1`: skip entries that already match by term, on the first conflict truncate the log from that index and append the remainder, append anything past the end. Never remove entries that match.
5. If `LeaderCommit > Commit`, set `Commit = min(LeaderCommit, PrevIndex+len(Entries))`. Commit never moves backwards (a duplicate or heartbeat with a smaller bound leaves it alone).
6. Reply `AppendResp{Term: f.Term, Success: true, Match: PrevIndex + len(Entries)}`.

Copy the entries into the follower's log; the caller may reuse the request's slice.

The hidden tests include hand-written scenarios (gaps, conflicts, duplicates, stale terms, commit clamping) and random streams of requests, sent out of order and duplicated, generated from several leaders' diverging histories. Each reply and the follower's state are compared with an independent model, and the log matching property is checked.
