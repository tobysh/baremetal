Split a 64-bit base into pieces with shifts and masks: `base & 0xFFFF`, `(base >> 16) & 0xFF`, `(base >> 24) & 0xFF`, `(uint32_t)(base >> 32)`. Byte 6 is `((limit >> 16) & 0x0F) | (flags << 4)`, and the flags are 0 here.
===
`tss_init` can start with `memset(t, 0, sizeof *t)`. Remember `iopb_offset` is a byte offset from the start of the TSS and must be at least the limit plus one, so `sizeof(Tss)` works.
===
In `gdt_setup` the order matters: fill the table (including the TSS descriptor, whose base is `(uint64_t)&tss` and limit `sizeof(Tss) - 1`), then `gdt_flush`, and only then `load_task_register(0x28)`. `ltr` needs the descriptor to be in the loaded GDT, and it marks it busy, so it must not run before the final table is in place.
===
The descriptor pointer for `gdt_flush` is a packed struct with a `uint16_t` size and a `uint64_t` base: `{ sizeof(gdt) - 1, (uint64_t)&gdt }`. With the 16-byte TSS descriptor, `sizeof(gdt)` is 56, so the size field is 55.
