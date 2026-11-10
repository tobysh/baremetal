Implement `Recover(d *Disk) error`, run after a restart.

- If `d.Journal.Committed` is false, do nothing and return `nil`: no writes, `Attempts` unchanged, journal untouched.
- Otherwise first validate **every** record: its `Block` must be in `0 .. len(d.Home)-1`. On any invalid address return `ErrJournal` before calling `d.Write` at all; the journal stays exactly as it was and `Home` is untouched.
- Then replay the records in order with `d.Write(r.Block, r.Value)`, including repeated addresses (the last value wins).
- If `d.Write` returns an error, stop at once and return that error. Do not write further records, and keep the whole journal (still `Committed`, same records) so that a later `Recover` can retry from the start.
- When every write has succeeded, clear the journal (`Committed = false`, no records) and return `nil`. A committed journal with no records is valid and is cleared too.
- A completed recovery is idempotent: calling `Recover` again makes no writes. `Recover` never changes `FailAt` or resets `Attempts`.

`FailAt` is compared with the running `Attempts` counter, so `FailAt = Attempts + k` makes the k-th next write crash. The tests crash recovery at every record, crash it repeatedly, and compare the final `Home` with a model.
