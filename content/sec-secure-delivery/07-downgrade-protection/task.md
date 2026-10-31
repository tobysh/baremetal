Implement `Decide(st State, o Offer, now int64) error` and `Apply(st State, o Offer) State` (types, `Skew` and the error variables are supplied). `Decide` returns the first matching error in the order above, or nil.

`main` walks through a public update history. The hidden tests use a table for every rule and boundary (exactly `now+Skew`, exactly `ExpiresAt`), scenarios in which an attacker replays old offers after a local rollback, and many random states and offers compared with an independently written rule list.
