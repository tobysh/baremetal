`write_full` is provided in the starter. Implement:

- `int tcp_connect(uint16_t port)` connects to `127.0.0.1:port` and returns the connected socket, or `-1` on failure (for instance connection refused). Do not leak a descriptor on failure.
- `int line_request(uint16_t port, const char *request, char *reply, size_t cap)`:
  1. connect to the port;
  2. send `request` followed by a single `'\n'`;
  3. read the reply up to and including the first `'\n'` (or until the server closes);
  4. store it in `reply` **without** the newline, NUL-terminated;
  5. close the socket and return the reply's length.

  Return `-1` if the connection or any I/O fails, or if the reply (plus its terminating NUL) would not fit in `cap` bytes. Close the socket on every path.
