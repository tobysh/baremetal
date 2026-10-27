The descriptor helpers from the last lesson, `load_gdt` and `load_idt` (inline assembly), the table arrays and the state variables `last_vector`, `last_error`, `last_rip` and `exception_count` are in the starter. Implement:

- `void gdt_install(void)` fills `gdt` with the null descriptor, a 64-bit kernel code segment (access `0x9A`, flags `0xA`) at selector `0x08` and a kernel data segment (access `0x92`, flags `0xC`) at `0x10`, then calls `load_gdt` with a descriptor pointer for the 3 entries.
- `void idt_install(void)` fills `idt[0..31]` with gates to `bm_isr_table[i]` (selector `0x08`, type `0x8E`, IST 0), then calls `load_idt` with a descriptor pointer for those 32 gates.
- `void exception_handler(uint64_t vector, uint64_t error_code, uint64_t rip)` stores the three values in `last_vector`, `last_error` and `last_rip` and adds 1 to `exception_count`.
- `void exceptions_init(void)` calls `gdt_install`, `idt_install`, and sets `bm_exception_hook` to `exception_handler`.
