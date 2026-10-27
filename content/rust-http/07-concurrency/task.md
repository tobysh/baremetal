Write `fn serve(listener: TcpListener, max_connections: usize)`.

- Accept exactly `max_connections` connections with `listener.accept()`.
- Handle each one by calling `handle_connection(stream)` on its **own thread** (`std::thread::spawn` with a `move` closure). Do not wait for one connection to finish before accepting the next.
- After accepting all of them, `join` every thread, then return.
- If `accept` fails, skip that attempt (it still counts toward `max_connections`) rather than panic.

`handle_connection`, `route`, `build_response` and `parse_request_line` are provided in the starter.

The test connects client A and sends nothing for over a second. Meanwhile client B must get its reply within one second. A sequential server cannot do that.
