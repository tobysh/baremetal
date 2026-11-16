Write the loop over `passwords` first, and make each iteration do exactly one login. Use `json.Marshal` on a `map[string]string` for the body.
===
Decide what a candidate is: status 200 and `Token != ""`. Everything else is `continue`. Do not request `/admin/proof` for non-candidates.
===
For a candidate, send `GET /admin/proof` with `Token` set to the candidate token. A decoy session answers 403, so the next password must still be tried.
===
Return only when the proof reply has status 200 and starts with `LAB-PROOF:`, using `Finding{port, proof.Body}`. After the loop, return `Finding{}, false`.
