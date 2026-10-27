The packed structs `GdtEntry`, `IdtGate` and `DescriptorPointer` are defined in the starter. Implement:

- `GdtEntry gdt_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)` splits `base` and the 20-bit `limit` across the entry as shown in the lesson. `flags` is a 4-bit value that goes in the high nibble of byte 6. Ignore limit bits above bit 19.
- `IdtGate idt_gate(uint64_t handler, uint16_t selector, uint8_t type_attr, uint8_t ist)` builds a 64-bit-mode gate. The reserved dword is 0.
- `DescriptorPointer descriptor_pointer(const void *table, size_t bytes)` returns the pointer for `lgdt`/`lidt`: size is `bytes - 1`, base is the table's address.
