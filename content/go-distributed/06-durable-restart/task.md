`Node` (supplied) has `HandleVote` and `HandleAppend`, which update its `Term`, `VotedFor` and `Log` in memory. Complete `Server`:

- `NewServer(d Disk)` restores the node from `d.Load()`: term, vote and log; `Commit` starts at 0.
- `Vote(r)` and `Append(r)` pass the request to the node and, **if the persistent state changed** (term, vote or log), call `d.Save` with the new state **before** returning the reply. If nothing persistent changed, do not call `Save`. The reply is exactly what the node produced.
- If `Save` returns an error, return that error and a zero reply, and put the node back to what the disk holds (so memory is never ahead of disk).

The hidden tests feed a server thousands of random vote and append requests, crash and restart it at random points (rebuilding it only from the disk), and make the disk fail at random. A shadow node, running the supplied logic without ever crashing, must agree with the server on every reply and on the persistent state after every restart. They also check from outside that no term ever has two granted candidates, that heartbeats cause no writes, and that a failed write leaves memory equal to disk.
