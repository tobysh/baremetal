`gdt[0] = gdt_entry(0, 0, 0, 0);` then the code segment `gdt_entry(0, 0xFFFFF, 0x9A, 0xA)` and the data segment `gdt_entry(0, 0xFFFFF, 0x92, 0xC)`. Pass the pointer with `descriptor_pointer(gdt, 3 * sizeof(GdtEntry))`.
===
For the IDT, loop `i` from 0 to 31 and write `idt[i] = idt_gate((uint64_t)(uintptr_t)bm_isr_table[i], 0x08, 0x8E, 0);`. The size passed to `descriptor_pointer` is `32 * sizeof(IdtGate)`, which is 512.
===
If the machine resets right after `lgdt`, the code segment is almost always wrong. Check that its flags are `0xA` (L bit set, D bit clear) and its access byte is `0x9A`.
===
`exception_handler` is only four assignments. Remember `exception_count++` so the tests can tell how many exceptions arrived.
