The `Ring` and port helpers, `idt_init`, `pic_remap`, `pic_eoi`, `pic_set_mask`, `pit_init`, `irq_save`, `irq_restore` and the global `Ring kbd_ring` are in the starter. Implement:

- `void ring_init(Ring *r)` empties the ring. `uint32_t ring_count(const Ring *r)` returns the bytes stored (`head - tail`).
- `int ring_push(Ring *r, uint8_t b)` stores `b` and returns 1, or returns 0 without changing the ring when it is full. `int ring_pop(Ring *r, uint8_t *out)` stores the oldest byte in `*out` and returns 1, or returns 0 without touching `*out` when empty. Both must work when the counters wrap past `2^32`.
- `void kbd_irq(void)` reads every waiting byte from the controller (status `0x64` bit 0), discards mouse bytes (status bit 5), and pushes keyboard bytes into `kbd_ring`.
- `void interrupt_handler(uint64_t vector, uint64_t error_code, uint64_t rip)` ignores vectors outside 32 to 47. For IRQ 0 it adds 1 to `ticks`; for IRQ 1 it calls `kbd_irq()`; spurious IRQs 7 and 15 get no EOI (except the master's EOI for 15); every other IRQ is just acknowledged. All real IRQs end with `pic_eoi`.
- `void keyboard_init(void)` empties `kbd_ring`, installs the IDT, remaps the PICs to 32 and 40, masks every IRQ except IRQ 1, sets `bm_exception_hook` to `interrupt_handler` and enables interrupts.
- `int kbd_read(void)` returns the next byte (0 to 255) or -1 if the ring is empty, without blocking. It must disable interrupts while it touches the ring and restore the previous state afterwards.
- `uint8_t kbd_wait(void)` returns the next byte, sleeping with `hlt` while the ring is empty, without ever missing a key that arrives between the check and the sleep. It returns with interrupts enabled.
