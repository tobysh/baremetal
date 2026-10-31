Implement the `Node` methods in `starter.go`. `VotedFor` is `-1` when the node has not voted in its current term.

- `StartElection()`: increment `Term`, become `Candidate`, vote for yourself and return the `VoteReq` to send to every other node (using the node's own `LastIndex`/`LastTerm`). If your own vote is already a majority (a one-node cluster), become `Leader` at once.
- `HandleVoteReq(r)`: if `r.Term` is higher than yours, adopt it, clear your vote and become a follower. Then grant the vote only if `r.Term` equals your term, you have not voted for anyone else in this term, and the candidate's log is at least as up to date as yours. Reply with your (possibly updated) term. Granting a vote records `VotedFor`.
- `HandleVoteResp(r)`: a higher term makes you a follower. Otherwise count a granted vote only if you are still a `Candidate` and the reply is for your current term, and count each voter once. A majority makes you `Leader`.
- `HandleHeartbeat(term, leader)` reports whether the heartbeat is accepted. A term lower than yours is rejected. Otherwise adopt the term (clearing your vote only if it is higher) and become a `Follower`.

The hidden harness runs 3 and 5 node clusters through many seeded schedules with random timeouts, dropped, duplicated and reordered messages and partitions. It checks that each term has at most one leader, that no leader lacks a majority of real grants, that no node votes twice in a term, that terms never go down, and that nodes with out-of-date logs never win.
