Allocate 12+len(code) bytes only after checking ranges. The checksum is over code alone.
===
Use encoding/binary and hash/crc32.ChecksumIEEE; copy code into newly allocated output.
