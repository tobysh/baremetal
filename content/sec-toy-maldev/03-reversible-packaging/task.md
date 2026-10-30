The package envelope is `BX01` (4 bytes), decoded length (2 little-endian bytes), nonzero XOR key (1 byte), IEEE CRC32 of decoded bytes (4 little-endian bytes), then each decoded byte XORed with the key. Header size is 11. Decoded length must be 1–8192 and match the envelope length exactly.

Implement `Pack(plain []byte, key byte) ([]byte, error)` and `Unpack(bundle []byte) ([]byte, error)`. Reject a zero key, invalid magic, wrong length, truncation, trailing bytes, empty decoded data, excessive decoded data or checksum mismatch. Return nil,error on any failure. Both functions must leave inputs unchanged and return independent newly allocated bytes on success. Pack operates on arbitrary inert bytes; capsule validation is a later layer.

