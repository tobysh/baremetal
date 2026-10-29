`EncodePut` and `EncodeDelete` in the starter write the record format shown in the lesson. Implement the recovery side.

- `Replay(log []byte) (state map[string]string, valid int)` applies the records in order (put sets the key, delete removes it) and returns the final state, which is never `nil`. It stops at the first record that is incomplete or whose op byte is not 1 or 2, and `valid` is the number of bytes occupied by the complete records before that point.
- `Recover(buf *bytes.Buffer) map[string]string` replays `buf.Bytes()`, **truncates** `buf` to the valid prefix so that later appends follow good data, and returns the state.

Keys and values are plain strings. A damaged tail must never cause a panic.
