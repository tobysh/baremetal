Implement the four functions in the starter exactly as the lesson describes.

- `sig_set_disp(p, sig, disp)`: `-1` unless `sig` is 1..31, not `SG_KILL`/`SG_STOP`, and `disp` is one of `D_DFL`, `D_IGN`, `D_CATCH`. After setting, if `is_ignored(p, sig)`, clear the pending bit.
- `sig_mask(p, how, set)`: clear `SG_KILL`, `SG_STOP` and bit 0 from `set`, apply `M_BLOCK` / `M_UNBLOCK` / `M_SETMASK`, return the old mask. Any other `how` changes nothing.
- `sig_send(p, sig)`: `-1` for an invalid number; a dead process ignores everything (return 0). Apply the `SIGCONT`/stop interactions, discard ignored unblocked signals, otherwise set the pending bit.
- `sig_deliver(p)`: nothing if dead. Choose the lowest signal that is pending and unblocked (only `SIGKILL` if the process is stopped), clear its bit, then act: kill, run the handler (log it in `handled[]`), ignore, stop, or terminate by default. Return the number.
