Implement `ExploitAgent(lab Lab, port int, marker string) (Finding, bool)`. The signature and types are fixed. `marker` contains only ASCII letters, digits, colon and hyphen, and begins `LAB-PROOF:`.

1. `GET /version` on `port`. Continue only for status 200 and body exactly `labagent/0.9`. Otherwise return `Finding{}`, `false` without sending anything else.
2. `POST /job` with a JSON body `{"label": "<payload>"}` (build it with `encoding/json`). Send exactly one job.
3. `GET /last-proof`, **whatever the job reply status was**. Return `Finding{Port: port, Evidence: body}` and `true` only for status 200 with a body exactly equal to `marker`. Otherwise return the zero `Finding` and `false`.

The vulnerable target interprets the string `"backup " + label` as semicolon-separated commands: `backup <anything>` has no effect and `emit <text>` stores `<text>` as the last proof. Your payload must therefore make the label end the first command and add an `emit` command carrying the supplied marker.

Rules: only these three operations, only on `port`, no token. A job status alone is never evidence, and a banner of `labagent/0.9` does not guarantee a vulnerable target.
