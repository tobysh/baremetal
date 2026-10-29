`tcp_connect` is `socket` + `connect`, with `close(s)` and `return -1` if connect fails.
===
In `line_request`, send the text with `write_full(s, request, strlen(request))` and then `write_full(s, "\n", 1)`.
===
Read one byte at a time: `while (read(s, &ch, 1) == 1) { if (ch == '\n') break; if (len + 1 >= cap) {fail;} reply[len++] = ch; }`. Then `reply[len] = '\0'`. Use a single exit point that always calls `close(s)`.
