Implement `probe_fields(target, baseline, fields, values, n)`.

First call `update(ctx, "display_name", "Lab Tester")`, then `read(ctx)`. The control passes only if update status is 200–299, the read succeeds, `display_name` equals `Lab Tester`, and both protected integers (`role`, `credit`) equal baseline. Return `-2` on control failure.

For each candidate in order call update once then read once, regardless of update status. Return `-2` immediately if a read fails. Return its zero-based index if either protected integer differs from baseline, otherwise continue. Return `-1` after all candidates leave protected state unchanged. Do not compare display_name for candidate probes. Always run the control even for n zero.

The supplied callback resets state to baseline before EVERY update, then applies that update. Reads have no side effects. Pointers are valid; strings are NUL-terminated, display_name fits in 32 bytes, and n is 0–32. Arrays may be NULL for n zero. No other calls are permitted. Hidden tests vary baselines, candidate fields, response statuses and persisted state.
