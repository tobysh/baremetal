Store `queue [][]byte`, a `queued` byte count and an `inflight` count. `Send`: `b, err := Encode(typ, payload)`; check the cap with `s.queued+len(b) > s.max` before touching any state.
===
`Next` pops `s.queue[0]`, subtracts its length from `queued`, increments `inflight`. Check the window first: `s.inflight >= s.window` means no credit.
===
`Ack`: reject `n < 0 || n > s.inflight` before changing anything. Set the popped slot to nil (`s.queue[0] = nil`) before reslicing so the queue does not pin old frames in memory.
