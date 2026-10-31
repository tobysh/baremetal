---
title: An Interrupt Controller
summary: Register handlers, enable lines through a simulated controller, and dispatch pending interrupts without losing events that arrive while a handler runs.
minutes: 35
difficulty: hard
tags: embedded, interrupts, dispatch
id: c-embedded/interrupt-controller
mode: harness
---
Polling wastes time; an **interrupt controller** lets devices announce events. It sits between the device lines and the CPU and has three registers:

| offset | name | meaning |
|---|---|---|
| 0x00 | ENABLE | bit n set = line n may interrupt (read/write) |
| 0x04 | PENDING | bit n set = an event on line n has been latched (read-only) |
| 0x08 | ACK | write-one-to-clear: writing bit n clears pending bit n |

Eight lines, numbered 0 to 7. An event latches PENDING whether or not the line is enabled; a masked line simply is not delivered *yet*.

## Ack before the handler

This controller is **edge-latching**: a new event while the bit is already set is merged into it. Suppose a handler for line 2 runs and, partway through, line 2 fires again. If the dispatcher acknowledges **after** the handler, the ack erases the new event, which is lost for ever. If it acknowledges **before** calling the handler, the new event sets PENDING again and the next pass picks it up. Always: ack, then run.

Acknowledge only the line you are about to run, never `ACK = 0xFF`: a masked or later line that is pending must stay pending.

## What you implement

```c
typedef void (*isr_t)(void *ctx);
typedef struct { isr_t isr[8]; void *ctx[8]; uint32_t spurious; } irq_table_t;   // all-zero = empty
```

- `int irq_register(irq_table_t *t, unsigned line, isr_t isr, void *ctx)` returns 0, or -1 if `line >= 8`, `isr` is NULL, or the line already has a handler.
- `void intc_enable(bus_t *b, unsigned line, int on)` sets or clears bit `line` of ENABLE with one read and one write, leaving the other bits alone.
- `int irq_dispatch(bus_t *b, irq_table_t *t)` runs up to **16 passes**. Each pass reads PENDING and ENABLE once and takes `pending & enabled`. If it is zero the dispatch is over. Otherwise, for each set line **in ascending order**: write ACK with just that bit, then call the handler (if none is registered, count it in `t->spurious`). Return the number of handlers called.

The pass limit protects against a stuck line that re-fires forever; the rest of the system must still get CPU time. **Limitation:** a real interrupt arrives asynchronously, in the middle of other code, and the dispatcher also needs to save context and deal with priorities and nesting. Here the tests call `irq_dispatch` at chosen points and handlers raise events through the simulator, so only the acknowledge ordering and bookkeeping are checked.

Good handlers do little: read the data, clear the cause, record the event, return. Anything slow belongs in the main loop. That rule cannot be machine-graded.
