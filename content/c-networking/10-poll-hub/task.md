Implement `int hub_step(int *fds, int n, int timeout_ms)`:

- Wait with `poll` for up to `timeout_ms` for input on any `fds[i] >= 0` among the `n` entries. Retry if `poll` is interrupted by a signal.
- If nothing is ready, return `0`.
- For every ready descriptor (readable, hung up or in error) do one `read` of up to 256 bytes.
  - If it returned `k > 0` bytes, write all `k` bytes to every **other** open descriptor (never back to the sender) and count one relayed message.
  - If it returned `0` or `-1`, `close` the descriptor and set `fds[i] = -1`.
- Return the number of messages relayed (the number of reads that returned data), or `-1` if `poll` fails.

Entries equal to `-1` must be ignored. A call must not block longer than `timeout_ms`, and must return promptly once any descriptor is ready.
