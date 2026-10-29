`ring_push`: `if (ring_count(r) == RING_SIZE) return 0; r->data[r->head & (RING_SIZE - 1)] = b; r->head++; return 1;`. `ring_pop` mirrors it with `tail`. `ring_count` is `r->head - r->tail`, which is correct across wrap-around because both are `uint32_t`.
===
`kbd_irq`: `while (inb(0x64) & 1) { ...read the status again so you have it with you... }`. For example `uint8_t st = inb(0x64); if (!(st & 1)) break; uint8_t b = inb(0x60); if (!(st & 0x20)) ring_push(&kbd_ring, b);`.
===
`interrupt_handler`: `if (vector < 32 || vector > 47) return; unsigned irq = vector - 32;`. For `irq == 7` return without any EOI. For `irq == 15` send only the master's EOI (`outb(0x20, 0x20)`) and return. Then `if (irq == 0) ticks++; else if (irq == 1) kbd_irq(); pic_eoi(irq);`. This lesson only unmasks IRQ 1 (and the test adds IRQ 0), so a real IRQ 7 or 15 never arrives; the rule is the defensive habit from the timer lesson.
===
`kbd_wait`: a loop of `__asm__ volatile("cli" ::: "memory"); if (ring_pop(&kbd_ring, &b)) { __asm__ volatile("sti" ::: "memory"); return b; } __asm__ volatile("sti; hlt" ::: "memory");`. `kbd_read` is `uint64_t f = irq_save(); int r = ring_pop(&kbd_ring, &b) ? b : -1; irq_restore(f); return r;`.
