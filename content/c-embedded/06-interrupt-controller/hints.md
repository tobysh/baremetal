`irq_register` is bounds and duplicate checks followed by two stores into the table.
===
`intc_enable`: `uint32_t e = b->read(b, ENABLE); e = on ? e | bit : e & ~bit; b->write(b, ENABLE, e);`
===
In the dispatch loop, `b->write(b, ACK, 1u << line)` must come before `t->isr[line](t->ctx[line])`. Wrap the pass in `for (pass = 0; pass < 16; pass++)` and `break` when nothing is pending and enabled.
