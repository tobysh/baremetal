Implement `uint16_t inet_checksum(const void *data, size_t len)`.

Read the data as little-endian 16-bit words, treating a final odd byte as a word with a zero high byte. Sum them, fold the carries into 16 bits, and return the one's complement. `len` may be 0 (the answer is 0xFFFF). Long buffers with many carries and every length from 0 up are tried; the data pointer is not necessarily aligned.
