For `gdt_entry`, assign each field from a shifted and masked piece: `limit_low = limit & 0xFFFF`, `base_low = base & 0xFFFF`, `base_mid = (base >> 16) & 0xFF`, `base_high = (base >> 24) & 0xFF`.
===
Byte 6 combines two things: `(flags << 4) | ((limit >> 16) & 0x0F)`. Mask both so stray high bits can't leak into the wrong nibble.
===
For `idt_gate`, `offset_low = handler & 0xFFFF`, `offset_mid = (handler >> 16) & 0xFFFF`, `offset_high = handler >> 32`. Write `0` into `zero` and `ist & 7` into `ist`.
===
`descriptor_pointer` is two assignments: `size = bytes - 1` and `base = (uint64_t)(uintptr_t)table`.
