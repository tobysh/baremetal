Start with the guard: read `/version` and return early unless it is a 200 with exactly `labagent/0.9`. Then there are only two more calls, the job and the proof read.
===
The target builds `"backup " + label` and splits on `;`. Think about what string for `label` makes that split yield a second, separate command that starts with `emit `.
===
Write the label as a harmless job name, then a semicolon, then `emit ` and the marker. Marshal it with `json.Marshal(map[string]string{"label": ...})`.
===
After posting, always read `GET /last-proof` yourself, and compare the body to the marker with `==`. Ignore the job reply's status: a patched agent returns the same `202` as a vulnerable one.
