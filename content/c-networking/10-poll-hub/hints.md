Build a `struct pollfd` array: `p[i].fd = fds[i]; p[i].events = POLLIN;`. A `-1` fd is skipped by `poll` automatically, so copy the array as it is. Use a variable-length array or `malloc` for the `n` entries.
===
After `poll` returns positive, loop `i` over the entries: `if (p[i].revents & (POLLIN | POLLHUP | POLLERR))`. Read into a `char buf[256]`. Remember the slot to clear is `fds[i]`.
===
Relaying: for each `j != i` with `fds[j] >= 0`, write the bytes. Do the relaying right after the read, before you process the next ready entry. If a relay write fails (EPIPE), you may ignore it or close that one; the test ignores `SIGPIPE`.
