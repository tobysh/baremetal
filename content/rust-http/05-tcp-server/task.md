Write `fn handle_connection(mut stream: TcpStream)`.

It must:

1. Read from the stream, accumulating bytes into a buffer, until the buffer contains `"\r\n\r\n"`, the client closes the connection (`read` returns `Ok(0)`), a read error occurs, or the buffer reaches 8192 bytes. A request may arrive in several separate pieces.
2. Take the first line of the received text (convert with `String::from_utf8_lossy`) and parse it with `parse_request_line`.
3. If it parses, call `route(&method, &path)` and send `build_response(status, "text/plain", &body)`.
4. If it does not parse (including empty input), send `build_response(400, "text/plain", "bad request")`.
5. Write the whole response with `write_all`, then return. Dropping the stream closes the connection.

`parse_request_line`, `route` and `build_response` are provided in the starter. Do not panic on bad input or I/O errors: ignore write errors.
