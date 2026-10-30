Build the 64-byte key block first. If `klen > 64`, hash the key into the first 32 bytes; otherwise copy it. The rest of the block stays zero, so an empty key is just 64 zero bytes.
===
Make two 64-byte arrays, `k[i] ^ 0x36` and `k[i] ^ 0x5c`. Inner digest: init, update with the inner pad, update with the message, final. Outer digest: init, update with the outer pad, update with the 32-byte inner digest, final into `out`.
===
For `hmac_verify`, return 0 at once if `taglen != 32` (the length is not secret). Then OR together `want[i] ^ tag[i]` over all 32 bytes and test the result against zero once at the end.
