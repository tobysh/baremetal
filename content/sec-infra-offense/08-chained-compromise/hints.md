Treat the config as data: validate its management port against the original scope before sending any request there.
===
Carry the leaked token to the management operation and require the returned marker. A leak alone is not yet the capstone proof.
===
Write the loop first: for each port, banner, config, value, snapshot, and `continue` at every failed gate. Only return when the snapshot proves access.
===
Build a set from `ports` and check `cfg.ManagementPort` against it before the value read, so an out-of-scope address never receives a request. Reject empty keys and empty tokens.
