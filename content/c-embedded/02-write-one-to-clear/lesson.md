---
title: Write One to Clear
summary: Acknowledge status flags that clear when you write a 1, and read a data register that pops a FIFO exactly once per byte.
minutes: 25
difficulty: medium
tags: embedded, registers, status-flags
id: c-embedded/write-one-to-clear
mode: harness
---
Not every register behaves like memory. Two contracts show up in nearly every device.

## Write-one-to-clear (W1C)

Status flags such as "overrun" or "parity error" are *set by the hardware*. Software cannot write a 0 to clear them; it writes a **1 to the bit it wants to clear**, and 0 bits are ignored. That makes acknowledgement safe without a read-modify-write:

```c
uint32_t st = b->read(b, STATUS);
uint32_t seen = st & ERR_MASK;
if (seen) b->write(b, STATUS, seen);   // clears exactly what we saw
```

Writing a constant such as `ERR_MASK` instead is a classic bug. If a *new* error arrives between your read and your write, you wipe out an event you never looked at. Writing back only the bits you observed leaves later events pending.

## Read-sensitive registers

Reading `DATA` pops one byte off the receive FIFO. Reading it a second time "just to look" throws a byte away. Always read it exactly once per byte, and only after `STATUS` says `RXNE` (receive not empty) is set. The upper 24 bits of `DATA` are not part of the byte; mask them.

## The simulated UART

| offset | name | bits |
|---|---|---|
| 0x00 | STATUS | 0 RXNE (read-only), 1 TXE (read-only), 2 OVERRUN, 3 PARITY, 4 FRAME (W1C) |
| 0x04 | DATA | read pops a byte (low 8 bits meaningful) |

The hidden simulator raises a new error flag right after your first `STATUS` read in some tests, which is how real hardware surprises a driver.

## Your turn

Implement `uart_clear_errors` and `uart_getc`. **Limitation:** the simulator models these two contracts only; a real UART has more registers and timing.
