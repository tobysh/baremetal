The packed structs `GdtEntry`, `TssDescriptor` and `Tss`, the helpers `gdt_entry` (from Part 1), `gdt_flush(const void *pointer)` (loads the GDT and reloads `cs`, `ds`, `es`, `ss`) and `load_task_register(uint16_t selector)` (the `ltr` instruction), and the `Gdt` layout are in the starter. Implement:

- `void tss_descriptor(TssDescriptor *out, uint64_t base, uint32_t limit)` fills a 16-byte 64-bit TSS descriptor as described in the lesson: access `0x89`, flags 0, the 20-bit `limit` split across bytes 0-1 and the low nibble of byte 6, the 64-bit `base` split across bytes 2-4, 7 and 8-11, and the reserved dword zero.
- `void tss_init(Tss *t, uint64_t kernel_stack_top)` zeroes the whole structure, sets `rsp0` to `kernel_stack_top` and `iopb_offset` to `sizeof(Tss)`.
- `void gdt_setup(uint64_t kernel_stack_top)` initialises the global `tss`, fills the global `gdt` with the six descriptors from the lesson (null, kernel code `0x9A`/`0xA`, kernel data `0x92`/`0xC`, user data `0xF2`/`0xC`, user code `0xFA`/`0xA`, then the TSS), loads it with `gdt_flush` using a pointer whose size is `sizeof(gdt) - 1`, and finally loads the task register with selector `0x28`.

All base and limit values are 0 for the code and data segments except the limit `0xFFFFF`, as in Part 1.
