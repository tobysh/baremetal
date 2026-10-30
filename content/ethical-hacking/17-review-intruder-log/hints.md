One array of per-source records (name, failure count, success text). While reading, an `OK` from a source whose failure count is already 5 or more, and which has no success yet, becomes its first suspected success.
===
For `PIVOTS`, keep the list of previous `OK` events and search backwards for a matching destination. A single `OK` counts once, however many earlier events match.
