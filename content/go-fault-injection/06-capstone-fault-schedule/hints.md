Start with the fault-free run: `d := NewFaultDisk(nil)`, open, do all puts, read `d.Ops()`. That is how many appends to target.
===
Write `runOne(open, puts, fault) []string` and loop over faults outside it. Inside, use `defer func(){ if r := recover(); r != nil { ... } }()` to turn a panic into a violation.
===
Track `acked map[string]string` and `inflight *KV`. After `d.Restart()` and a reopen, compare every key in `puts` against the allowed values. An absent key is allowed only if it was never acknowledged.
===
For invariant 4, record what the first reopen showed, put `post-recovery`, restart, reopen again, and compare with the earlier observations plus the new key. Two correct engines differ in where they split writes, so never assume the log layout.
