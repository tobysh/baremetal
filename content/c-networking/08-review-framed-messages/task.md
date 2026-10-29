The helpers `read_full`, `write_full`, `put_u16` and `get_u16` are in the starter. Frames are a 2-byte big-endian length followed by that many payload bytes. The largest payload is `MAX_FRAME` (1024) bytes. Implement:

- `int send_frame(int fd, const uint8_t *payload, size_t n)` sends one frame and returns `0`. Return `-1` if `n > MAX_FRAME` (send nothing) or on any write error.
- `int recv_frame(int fd, uint8_t *buf, size_t cap)` receives one frame into `buf` and returns the payload length (which may be `0`). Return `-2` if the peer closed cleanly before the first header byte. Return `-1` on an I/O error, on a connection closed in the middle of a frame, on a length above `MAX_FRAME`, or if the frame is larger than `cap` (never write past `cap`).
- `int serve_reversed(int fd)` loops: receive a frame, send back a frame holding the same bytes **in reverse order**. When `recv_frame` reports a clean close, return the number of frames served. If any other error happens, return `-1`.
