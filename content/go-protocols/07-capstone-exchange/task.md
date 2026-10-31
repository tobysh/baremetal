All earlier code and the wire constants are given. Implement `Client` and `Server`.

**Server**
- `NewServer(h Hello) *Server`
- `OnData(p []byte) []byte`: feed received bytes, return the concatenated reply frames (nil if none).
- `Closed() bool` after BYE.

**Client**
- `NewClient(h Hello, reqs [][]byte) *Client` (each body at most 1020 bytes)
- `Tick(now int) []byte` and `OnData(now int, p []byte) []byte` return bytes to transmit
- `Done() bool`, `Err() error`, `Results() [][]byte` (nil entries until answered), `Version() uint8`, `Features() uint32`

Follow the lesson's behaviour exactly. Hidden tests run dozens of seeded fault schedules, a dead link, an incompatible server, random garbage and forged responses, and check that no more than `Window` requests are ever unanswered.
