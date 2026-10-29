Implement `DB` using the frame and payload layout from the lesson.

- `Open(log []byte) *DB` replays `log`. It stops at the first frame that is incomplete, has a wrong checksum, has an unknown op byte, or has a payload too short to hold its key. Everything after that point is thrown away. The DB keeps its **own copy** of the valid prefix (it must not alias the argument).
- `(*DB) Put(key, value string)` and `(*DB) Delete(key string)` append one frame to the log and update the state. `Delete` of a missing key still appends a frame.
- `(*DB) Get(key string) (string, bool)`.
- `(*DB) Log() []byte` returns a copy of the current log bytes.
- `(*DB) Compact()` replaces the log with one put frame per live key, sorted by key. The state does not change.
