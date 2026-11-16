Start with the outer loop over `ports`, and decide first what makes you skip a port. Skipping with `continue` keeps later ports reachable.
===
Treat the banner as a gate: status 200 and the exact text `labkv/1.0`, nothing else. Do not send `/keys` before it passes.
===
Inside a store, loop over `keys.Items` in order and build the path with `"/value/" + key`. Check both the status and `strings.HasPrefix(body, "LAB-PROOF:")`; public keys return ordinary text.
===
Return as soon as one value passes the check, and `Finding{}, false` after the loops. A hardened store answers `/keys` with 401, which should move you to the next port rather than end the search.
