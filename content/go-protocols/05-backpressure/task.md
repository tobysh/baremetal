The codec is given. Implement `Sender`:

- `NewSender(maxQueueBytes, window int) *Sender`
- `Send(typ byte, payload []byte) error`: encodes the frame immediately (so the payload is copied). `ErrTooLarge` if the payload is over `MaxPayload`; `ErrQueueFull` if queued bytes plus this encoded frame would exceed `maxQueueBytes` (exactly equal is allowed). On error nothing changes.
- `Next() ([]byte, bool)`: if the queue is non-empty and in-flight frames < `window`, pop the oldest encoded frame, count it as in flight and return it; otherwise `nil, false`.
- `Ack(n int) error`: `n` frames were received. `ErrBadAck` if `n < 0` or `n > InFlight()`; otherwise in-flight decreases by `n`.
- `InFlight() int`, `QueuedFrames() int`, `QueuedBytes() int` (sum of encoded sizes still queued, not in flight).

Hidden tests drive thousands of random operations against a separate simple model, and run a slow-consumer simulation with a producer that backs off on `ErrQueueFull`.
