Fields: `hdr []byte`, `inPayload bool`, `err error`, `n, done uint64`, `tab *canonTable`, `code uint64`, `clen int`.
===
In the header phase: `d.hdr = append(d.hdr, p...)`, then `parseHeader(d.hdr)`. On `errors.Is(err, ErrTruncated)` return `nil, nil`. On success take `p = d.hdr[used:]` and switch phase.
===
Per payload byte: if `done == n` already, that is trailing data (`ErrCorrupt`). Per bit: if `done == n`, the bit must be 0 (padding). Otherwise shift it into `code`, `clen++` and ask `tab.lookup(code, clen)`.
===
On a hit append the symbol, `done++`, reset `code` and `clen`. If `clen` passes `MaxCodeLen` it is `ErrCorrupt`.
===
Remember to store the error in the struct before returning it so the next call repeats it.
