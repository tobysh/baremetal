Make a table with one row per capture pair. Compare the request and reply operation codes, sequence numbers and lengths column by column.
===
The check byte is the xor of all the bytes before it, further xored with 0x5A. Operations 1 to 4 are echo, reverse, sum of the payload bytes as a two-byte big-endian number, and upper-casing letters a to z only. Replies use the request operation plus 0x80.
===
Errors use operation 0xff with a one-byte payload: 1 unknown operation, 2 bad check byte, 3 length field does not match the frame size. Frames shorter than five bytes or with a first byte other than 0xb7 are dropped. Check the length first, then the check byte, then the operation.
