Port helpers, an IDT for vectors 0 to 47, `sti`/`cli`, the `ticks` counter and `timer_ticks()` are in the starter. Implement:

- `void pic_remap(uint8_t offset1, uint8_t offset2)` runs ICW1 to ICW4 on both PICs (master offset `offset1`, slave offset `offset2`) and **preserves both interrupt masks**.
- `void pic_eoi(unsigned irq)` acknowledges `irq` (0 to 15): EOI to the slave as well when `irq >= 8`.
- `void pic_set_mask(unsigned irq, int masked)` sets or clears the mask bit for `irq` (0 to 15) on the right chip without changing the other bits.
- `void pit_init(uint32_t hz)` programs PIT channel 0 to fire `hz` times a second.
- `void interrupt_handler(uint64_t vector, uint64_t error_code, uint64_t rip)` ignores vectors outside 32 to 47. For IRQs it ignores spurious IRQ 7 and 15 (no `ticks`, no EOI except the master's for 15), adds 1 to `ticks` for IRQ 0, and acknowledges the IRQ.
- `void timer_init(uint32_t hz)` installs the IDT, remaps the PICs to 32 and 40, masks every IRQ except IRQ 0, starts the PIT at `hz`, sets `bm_exception_hook` to `interrupt_handler` and enables interrupts.
- `void sleep_ticks(uint64_t n)` returns after `n` more ticks, using `hlt` while it waits. `sleep_ticks(0)` returns at once.
