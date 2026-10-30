Implement `ProcessTasks(raw []byte, session string, last uint64) ([]TaskReply, uint64, error)`. Decode one JSON array containing 1–16 Task objects with only the fields in the starter. Reject unknown fields and any trailing JSON value or junk. Caller session must be nonempty.

Every task must have the exact session and sequence `last+1`, then `last+2`, and so on, with no uint64 wraparound. Supported kinds:

- `echo`: Data is a byte string of at most 255 bytes. Return TaskReply with the same Sequence, Kind `echo`, and unchanged Data.
- `stop`: Data must be empty and this must be the last task. Return TaskReply with the same Sequence, Kind `stopped`, and empty Data.

Stop is optional. Return replies and the final accepted sequence on success. On **any** error return nil, the original last, and an error. Never mutate raw. JSON null, an empty array, unknown kinds and replay are errors. Extra whitespace is fine.

Example: session `lab-a`, last 7, batch `[{"session":"lab-a","seq":8,"kind":"echo","data":"hi"},{"session":"lab-a","seq":9,"kind":"stop","data":""}]` yields echo and stopped replies with final sequence 9.

