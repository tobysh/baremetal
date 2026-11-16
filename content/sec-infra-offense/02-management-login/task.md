Implement `GuessManagement(lab Lab, port int, user string, passwords []string) (Finding, bool)`. The signature and types are fixed.

For each password in the order given (at most 16, each tried at most once):

1. `POST /login` on `port` with a JSON object `{"user": ..., "password": ...}` (lowercase field names, built with `encoding/json`) and an empty `Token`.
2. A reply of status 200 with a non-empty `Token` is only a **candidate** session. Any other reply: try the next password.
3. For a candidate, `GET /admin/proof` on the same port using that token. Only status 200 with a `Body` beginning `LAB-PROOF:` confirms management access. Return `Finding{Port: port, Evidence: body}` and `true` and stop trying passwords.
4. If the proof is refused or has another body, discard the token and continue.

If no password gives a confirmed proof (or the list is empty), return the zero `Finding` and `false`.

Rules: no ports, users or passwords other than the ones supplied; the evidence is the marker body, never a password or token.
