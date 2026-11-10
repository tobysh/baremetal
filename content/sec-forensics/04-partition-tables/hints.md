Slot `i` starts at `sector + 446 + 16 * i`. Look at the signature before the slots, and return -1 straight away when it is wrong.
===
Write a little-endian helper that combines four bytes. Cast each byte to `uint32_t` before shifting; a plain `p[3] << 24` is a signed shift and goes wrong for values of 0x80 and above.
===
Keep a separate output index. A slot is skipped when its type is zero or its sector count is zero, and the next used slot goes into the next free output position, so the output has no gaps.
===
`type_name` is a `switch` or a short table. Remember that only the exact byte 0x80 marks a partition bootable, and that 0x00 and every unlisted value are "Unknown".
