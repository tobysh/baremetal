Read stdin (at most 1000 lines; fields without spaces). For every leaf event whose chain has two or more events, print one line:

`CHAIN ORIGIN > H1 > H2 [> ...] users=U1,U2 start=TS1 end=TSN hops=N`

`ORIGIN` is the SRC of the first event; `H1..HN` are the DST of each event from first to leaf. `users` lists the distinct users in order of first appearance, comma-separated. `start` is the first event's TS, `end` the leaf's TS and `hops` the number of events. Chains are printed in the input order of their leaves. If no chain qualifies print `no lateral movement found`.

Tests include broken chains (gaps over 600 s), failures, and changes of user along the way.
