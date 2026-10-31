Implement `build_locked_push`, `build_flawed_push` and `build_cas_push` (each takes the program and the thread count `n`, and the starter's `push_base` already sets up the spec). Point `p->code[t]` at an `insn` array ending in `OP_END` for each thread.

Requirements checked by the hidden tests, for `n` = 1, 2 and 3:

- Spec fields unchanged; `init_mem` all zero; every thread has code ending in `OP_END`.
- Locked: uses `OP_LOCK`; no failing schedule.
- Flawed: uses neither `OP_LOCK` nor `OP_CAS`; correct when threads run one after another; with `n = 1` it is fine; with `n >= 2` your checker's counterexample must equal the reference's minimal counterexample.
- Corrected: uses `OP_CAS`, no lock; no failing schedule, retry loop included.

Then run the demo and read the flawed counterexample.
