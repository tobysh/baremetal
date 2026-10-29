`udp_bind` is `socket(AF_INET, SOCK_DGRAM, 0)`, fill the `sockaddr_in` with `htons(port)` and `htonl(INADDR_LOOPBACK)`, `bind`, then `getsockname` to learn the port.
===
`upper_reply_once`: `recvfrom(fd, buf, sizeof buf, 0, (struct sockaddr *)&from, &fl)`, convert each byte with `toupper` (or a range test), then `sendto(fd, buf, got, 0, (struct sockaddr *)&from, fl)`. The sender's port is `ntohs(from.sin_port)`.
===
`udp_request` can set `SO_RCVTIMEO` to a `struct timeval` of `timeout_ms / 1000` seconds and `(timeout_ms % 1000) * 1000` microseconds, then call `recv`. If it returns a negative number (timeout), return -1. The client socket does not need `bind`; the first `sendto` gives it a port automatically.
