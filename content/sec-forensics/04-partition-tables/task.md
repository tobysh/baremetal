Keep `typedef struct {int bootable; uint8_t type; uint32_t start_lba, sectors;} Partition;`.

Implement `int parse_mbr(const uint8_t *sector, Partition out[4])`. `sector` points to exactly 512 bytes.

- If `sector[510] != 0x55` or `sector[511] != 0xaa`, return -1 and do not rely on `out`.
- Otherwise look at the four 16-byte slots starting at offset 446, in order. Skip a slot whose type byte is zero or whose sector count is zero. Store each remaining slot into `out` in order with no gaps: `bootable` is 1 only when byte 0 equals `0x80`; `type` is byte 4; `start_lba` is bytes 8..11 and `sectors` bytes 12..15, both little-endian unsigned. Return the number stored (0 to 4).
- Bytes 0..445 (boot code) and the unused bytes of a skipped slot have no effect.

Implement `const char *type_name(uint8_t type)`: `0x07` "NTFS/exFAT", `0x0b` "FAT32 (CHS)", `0x0c` "FAT32 (LBA)", `0x82` "Linux swap", `0x83` "Linux", `0x05` "Extended", `0xee` "GPT protective", `0xef` "EFI System"; every other value (including 0x00) is "Unknown". Return string literals exactly as spelled.

The hidden tests include swapped and missing signatures, gaps between used slots, zero type with nonzero size and the reverse, boot indicators other than 0x80, values with the high bit set and all 256 type bytes. Keep the supplied type and function names.
