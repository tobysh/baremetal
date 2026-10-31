Start with `Verify`: key length and `ed25519.Verify` for the signature, then `ParseMeta`, then build an `Offer` from the metadata for `Decide`, and last compare `len(Body)` and `digestOf(Body)` with the metadata.
===
`Boot` and `Rollback` first: read the active file with `readActive`, compare `digestOf(slotBytes)` with the recorded digest. `Rollback` writes `Active{Cur: *a.Prev}` (no previous any more) with `atomicWrite`, after `raiseHW`.
===
`Recover`: remove `active.tmp` and `hw.tmp` only when `d.Read` shows they exist (a needless `Remove` counts as a mutating operation), then `raiseHW(d, cur.Counter, cur.Issued)` if there is an active file.
===
`Install` order: `Recover`, `LoadState`, `Verify`, choose the spare slot (`otherSlot(cur.Slot)`, or `SlotA` on a blank device), `d.Write` the body and read it back, `atomicWrite` the new `active` with `Prev` set to the old `Cur`, `raiseHW`, health check.
===
If a schedule breaks your solution, print the operation numbers: run an install on a fresh disk, note `d.Ops`, then put the failing fault at each number. Ask "if the process died right after this operation, what would `Boot` see?"
