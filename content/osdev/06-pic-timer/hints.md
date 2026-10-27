`pic_remap`: save both masks with `inb(0x21)` and `inb(0xA1)` first. Then write `0x11` to both command ports, the offsets to both data ports, `4` to the master and `2` to the slave, `0x01` to both, and finally restore the saved masks.
===
`pic_set_mask`: choose the data port with `irq < 8 ? 0x21 : 0xA1` and the bit with `irq & 7`. Read the old value, set or clear that one bit, write it back.
===
The handler must acknowledge the IRQ every time or no second tick ever arrives. For vectors 39 and 47 first write `0x0B` to the command port and read it back; if bit 7 is clear, return without counting.
===
`sleep_ticks`: `uint64_t start = ticks; while (ticks - start < n) __asm__ volatile("hlt");`. `ticks` is `volatile`, so it is re-read on each loop.
===
If no ticks ever arrive, check in order: is the offset 32 (the CPU will see vector 8 otherwise)? Is IRQ 0 unmasked (mask bit 0 clear)? Did `timer_init` call `sti`?
