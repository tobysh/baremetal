`Recover` has four stages: skip an uncommitted journal, validate, replay, clear. Write them as four separate blocks so you can see which stage may change what.
===
Validation is its own loop over `d.Journal.Records` that checks `0 <= r.Block < len(d.Home)` and returns `ErrJournal` without calling `d.Write`. Only when the loop finishes do you start writing, so a bad record in last position cannot leave half a transaction behind.
===
Replay with a second loop that calls `d.Write(r.Block, r.Value)` and `return`s the error straight away if it is non-nil. Do not touch `d.Journal` on that path: the retry needs every record, including the ones that already succeeded.
===
After the replay loop, and only there, set `d.Journal = Journal{}`. Because each record assigns a value, replaying from the first record again after a crash is harmless. Do not touch `FailAt` or `Attempts`, and do not write to `d.Home` directly; always go through `d.Write`.
