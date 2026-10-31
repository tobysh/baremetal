Implement `Verify(findings []Finding, snap Snapshot, fixedAt int64, iocIPs []string, activity []Activity) ([]Result, bool)`.

A `Finding` is `{ID, Kind, Ref string; Version int}`; kinds are `account`, `key`, `file` and `credential`. `Snapshot` has `Time`, `Covers map[string]bool` (kinds it inspected), `Accounts, Keys, Files []string`, `CredVersion map[string]int` and `OldRejected map[string]bool`.

Per finding, in order, `Result{ID, Status}`:
- `"unverifiable"` if `snap.Time < fixedAt`, or `!snap.Covers[kind]`.
- account/key/file: `"closed"` if Ref is absent from the matching list, else `"open"`.
- credential: `"unverifiable"` if Ref has no entry in `CredVersion`; `"closed"` if `CredVersion[Ref] > Version` and `OldRejected[Ref]`; otherwise `"open"`.

The second result is true if any `activity` entry (`{Time, IP}`) has an IP in `iocIPs` and `Time > fixedAt`.

Public example: a file finding with a snapshot taken after the fix, listing no such file, is closed.
