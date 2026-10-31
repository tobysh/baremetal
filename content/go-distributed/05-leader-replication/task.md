`Follower` and `HandleAppend` from the previous lesson are supplied. Implement the `Leader`. Peers are numbered `0..N-1`; the leader's own number is `ID` and it never sends to itself.

- `NewLeader(id, n, term, log)` copies the inherited `log`, starts with `Commit = 0`, `match = 0` for every peer and `next = len(log)+1`.
- `Propose(cmd)` appends `Entry{Term: l.Term, Cmd: cmd}` and returns its index (1-based). In a one-node cluster this commits immediately.
- `Request(peer, max)` builds the `AppendReq` for `peer`: its `Term`, `Leader`, `PrevIndex = next[peer]-1`, the matching `PrevTerm` (0 when `PrevIndex` is 0), up to `max` entries starting at `next[peer]` (all of them when `max <= 0`; copy them) and `LeaderCommit = l.Commit`. Nothing to send gives a heartbeat with no entries.
- `OnReply(peer, req, resp)` processes a reply to `req`. A `resp.Term` above `l.Term` sets `Deposed` and returns. On success raise `match[peer]` to `req.PrevIndex+len(req.Entries)` if higher and keep `next[peer] >= match[peer]+1`. On failure set `next[peer] = min(next[peer], resp.Match+1)` but never below `match[peer]+1`. Then advance the commit index.
- Commit rule: the largest index `i > Commit` such that `Log[i-1].Term == l.Term` and the leader plus the peers with `match >= i` form a majority (`N/2+1`).

The hidden tests drive the leader and real `Follower`s through seeded schedules with dropped, duplicated and reordered requests and replies, diverged follower logs and tiny batch sizes. After every step they check, from the followers' actual logs, that every committed entry is stored on a majority, that the commit index only moves forward and only lands on a current-term entry, and that the cluster converges once the network settles.
