Use two arrays of 1024 bytes: `buf` for values and `seen` for presence. A segment at `seq` lands at `seq - base`; reject `seq < base` before subtracting.
===
When copying a segment, compare against `seen` first: if the byte is already present and differs, return -1 (conflicting overlap). Track `end` as the maximum of `offset + n` over all segments.
===
After all segments are placed, check every position `0..end-1` is present and is printable or CR/LF. Only then look at the text: it must start with `GET `, the path runs to the next space, and ` HTTP/1.1\r\n` must follow it.
===
Do the length check on the path last: you need `path_len < cap` for the terminator. Copy into `out` only once everything has passed. Remember `delta > 1024 || n > 1024 - delta`, which cannot wrap, instead of `delta + n > 1024`.
