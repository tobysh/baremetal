---
title: Bounded Polling
summary: Wait for a device flag with a deadline measured on a wrapping tick counter, so a dead device cannot hang the firmware.
minutes: 25
difficulty: medium
tags: embedded, polling, timeouts
id: c-embedded/bounded-poll
mode: harness
---
`while (!(read(STATUS) & READY)) {}` is the most common line in beginner drivers, and the most dangerous. If the device never answers (unplugged, wedged, clock off) the firmware hangs forever. Every wait on hardware needs a **deadline**.

## Ticks and wraparound

Firmware usually has a free-running tick counter, here `b->now(b)`, a `uint32_t` that wraps from `0xFFFFFFFF` to `0`. Never compare absolute times (`now > deadline`) because the counter may wrap in the middle of a wait. Compare *elapsed* time instead; unsigned subtraction wraps correctly:

```c
uint32_t start = b->now(b);
...
if ((uint32_t)(b->now(b) - start) > timeout) /* give up */;
```

This works for any wait shorter than 2^32 ticks, even across the wrap.

## The contract for `wait_flag`

`int wait_flag(bus_t *b, uint32_t off, uint32_t mask, uint32_t timeout)` returns 0 once `(read & mask) == mask` and -1 on timeout.

1. Record `start = now` on entry.
2. Read the register. If the mask is satisfied, return 0.
3. Otherwise read the tick counter. If more than `timeout` ticks have elapsed since `start`, return -1; else go to 2.

So the register is **always read at least once**, even with `timeout == 0`, and the last look happens before giving up. Do not write to the device.

## The simulator

In the hidden tests each status read takes a fixed number of ticks (1, 3 or 7), modelling a slow bus, and the device becomes ready at a scheduled tick. Start values include `0xFFFFFFF0`, so the counter wraps mid-wait. The expected outcome and the number of reads are computed by a closed-form formula, not by running your code twice. **Limitation:** ticks here advance only on reads. A real counter runs independently, and a deadline check cannot interrupt a bus access that itself hangs.

## Your turn

Implement `wait_flag`. Passing the lesson unlocks a **challenge**: a combined deadline across two waits.
