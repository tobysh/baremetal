Implement `CheckEngine(open Opener, puts []KV) []string` in the starter. Everything else in the starter (`FaultDisk`, `LogStore`, the types) is given and should not need changes.

It returns one human-readable string per violation found, and nil when none is found. Each string should name the fault and the invariant, for example `Crash at append 3 keep=half: acknowledged key "a" lost`.

Follow the schedule and the five invariants in the lesson. In particular:

- Learn the number of appends from a fault-free run; engines that split a record into several appends have more.
- Use a fresh `FaultDisk` and a fresh engine for every scheduled fault.
- After a failed put, stop issuing puts, call `d.Restart()` and reopen.
- Accept the in-flight value where the lesson allows it: reporting it as a violation would be a false alarm.
- Convert panics into violations.

The tests call your checker with different put lists on a correct engine, a second correct engine that writes each record in two appends, and five broken engines (one per class of mistake). It must return nothing for the correct engines and something for every broken one.
