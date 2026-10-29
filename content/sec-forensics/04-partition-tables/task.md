Keep `typedef struct {int bootable;uint8_t type;uint32_t start_lba,sectors;} Partition;`. Implement `int parse_mbr(const uint8_t *sector,Partition out[4])`, returning -1 for a bad signature or the number of nonempty slots. Implement `const char *type_name(uint8_t type)`: 07 NTFS/exFAT, 0b FAT32 (CHS), 0c FAT32 (LBA), 82 Linux swap, 83 Linux, 05 Extended, ee GPT protective, ef EFI System, else Unknown.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
