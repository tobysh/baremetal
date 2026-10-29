Decode the two-byte type as `(p[12] << 8) | p[13]`.
===
If it is 0x8100, require 18 bytes before reading the inner type.
