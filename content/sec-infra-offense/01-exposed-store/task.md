Implement `DiscoverStore(lab Lab, ports []int) (Finding, bool)`. The signature and types are fixed.

For each port, in the order given:

1. `GET /banner`. Continue with this port only if the reply is status 200 and the body is exactly `labkv/1.0`. Otherwise send nothing more to it.
2. `GET /keys`. Continue only on status 200; `Items` holds the key names.
3. For each key in order, `GET /value/<key>`. The first reply with status 200 and a `Body` beginning `LAB-PROOF:` is the proof. Return `Finding{Port: port, Evidence: body}` and `true`.

If a port gives no proof (hardened, wrong banner, no `LAB-PROOF:` value), move on to the next port. If no port gives a proof, return the zero `Finding` and `false`.

Rules: every request is a `GET` with an empty `Token` and `Body`; you may contact only the supplied ports; do not read further keys after the first proof; a banner alone is never a finding. Expected values change in every test, so do not hardcode them.
