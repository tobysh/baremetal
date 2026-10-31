Write a helper `becomeFollower(term)` that sets `Term`, clears `VotedFor` to -1, sets `Role = Follower` and drops the vote set. Call it whenever a message carries a higher term.
===
In `HandleVoteReq`, handle the higher term first, then decide. The log test is `r.LastTerm > n.LastTerm || (r.LastTerm == n.LastTerm && r.LastIndex >= n.LastIndex)`.
===
Count votes in a `map[int]bool` keyed by voter id, so a duplicate reply cannot count twice. Start it in `StartElection` with your own id.
===
A candidate that receives a heartbeat with its own term has lost: become a follower but keep `VotedFor` (you voted for yourself in this term).
