Write a helper that calls `Get` on every peer and returns the best `Versioned` seen and how many calls succeeded. `Write` and `Read` both start with it.
===
In `Write`, keep a field `last` on the register. New version = `max(best.Ver, last) + 1`. Store it into `last` **before** sending any `Put`.
===
Count acknowledgements by `err == nil`. Do not stop at the first failure: try every peer, then compare the count with the quorum.
===
`Read`: after choosing the best, `Put(best)` to every peer (the `Replica` ignores older or equal versions) and require a quorum of nil errors before returning `best.Val`.
