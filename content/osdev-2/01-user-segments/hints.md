A 64-bit TSS descriptor is a normal GDT entry whose base and limit are scattered over several fields, plus an extra dword for base bits 32-63. The CPU reads the fields exactly where the lesson's layout says, so most of the work is slicing numbers into the right pieces.
===
Look at the three functions separately. `tss_descriptor` is pure bit slicing, `tss_init` is a zero-and-two-fields function, and `gdt_setup` only has to do things in the right order using the helpers the starter already gives you. Check the field sizes of `Tss` and the 16-byte `TssDescriptor` in the starter.
===
Typical mistakes: forgetting that byte 6 holds the top 4 limit bits as well as the flags, giving `iopb_offset` a value smaller than the limit, running `ltr` before the new GDT is loaded (the descriptor must be in the active table), and a GDT pointer size that is the entry count instead of bytes minus one.
===
Slicing example for a different layout: a 24-bit address stored as a low 16-bit field and a high 8-bit field is `lo = a & 0xFFFF; hi = (a >> 16) & 0xFF;`.
