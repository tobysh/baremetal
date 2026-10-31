Take a snapshot of `Term`, `VotedFor` and a copy of `Log` before calling the node, and another after. If they differ, `Save` the second one.
===
Write one helper `run(func()) error` used by both `Vote` and `Append`, so the ordering "change, save, then return" lives in one place.
===
Copy the log (`append([]Entry(nil), n.Log...)`) when building a `Persistent`. If you store the node's own slice, later in-place edits change what you think you saved.
===
On a failed `Save`, rebuild the node from `d.Load()` with the same code `NewServer` uses.
