Write each sample as a column of bytes and mark the ones you can explain: magic, text, length bytes.
===
The header is the magic, a one-byte version and a two-byte record count. A record is a level byte, a four-byte timestamp, a one-byte text length and then the text.
===
Both multi-byte numbers are little-endian: the least significant byte comes first. Check `pos + 6 <= size` and then `pos + 6 + len <= size` before reading.
