`write_full` is provided in the starter. Implement:

- `int tcp_listen(uint16_t port, uint16_t *bound_port)` creates a TCP socket, sets `SO_REUSEADDR`, binds it to `127.0.0.1:port` (`port` 0 means any free port), calls `listen` with a backlog of at least 8, stores the actual port in `*bound_port` (use `getsockname`) and returns the descriptor. On any failure close what you opened and return `-1`.
- `long echo_serve_one(int listen_fd)` accepts one connection, echoes every byte it reads back to the client until the client closes its sending side (`read` returns 0), closes the connection, and returns the total number of bytes echoed. Return `-1` if `accept` or a read or write fails.

The listening socket stays open after `echo_serve_one` returns, ready for the next client.
