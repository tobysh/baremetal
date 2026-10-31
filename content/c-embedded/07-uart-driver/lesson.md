---
title: "Capstone: A Buffered UART Driver"
summary: Combine register fields, W1C errors, bounded waits and interrupt handlers into a UART driver with ring buffers and error counters.
minutes: 50
difficulty: hard
tags: embedded, uart, ring-buffer, capstone
id: c-embedded/uart-driver
mode: harness
---
This capstone reuses everything in the track. Interrupt handlers must be short and must not wait, so the driver decouples the hardware from the program with two **ring buffers**: the ISR fills the receive ring, the main program drains it; the main program fills the transmit ring, the ISR drains it.

## The simulated UART

| offset | name | bits |
|---|---|---|
| 0x00 | STATUS | 0 RXNE, 1 TXE, 2 OVERRUN (W1C), 3 FRAME (W1C), 4 RESETTING (all others read-only) |
| 0x04 | DATA | read pops the receive FIFO (low 8 bits); write sends one byte if TXE, otherwise the byte is **lost** |
| 0x08 | CTRL | bit 0 RESET (write 1 to start; CTRL itself is ignored while RESETTING), bit 1 RXIE, bit 2 TXIE |

The receive FIFO holds 4 bytes; a fifth byte is dropped and sets OVERRUN. Sending a byte takes a couple of ticks, during which TXE is low. The interrupt line is high while `RXIE` and a byte is waiting, or `TXIE` and TXE, or **any error flag is set** (so an ISR that forgets to acknowledge errors is caught). Reset takes a configurable number of ticks, clears CTRL and the FIFO, and leaves stale error flags set. Each STATUS read costs a tick.

## The driver (the structs are fixed; tests inspect them)

```c
#define UART_RING 8                       // a power of two
typedef struct { uint8_t buf[UART_RING]; uint32_t head, tail; } ring_t;   // free-running counters
typedef struct { bus_t *bus; ring_t rx, tx; uint32_t overruns, frame_errors, rx_dropped; int txie; } uart_t;
```

`head` is where the next byte is written, `tail` where the next byte is read; both only ever increase, so `head - tail` is the fill level even after the counters wrap past 2^32. Index with `& (UART_RING - 1)`. A ring holds exactly `UART_RING` bytes. The tests start rings near the wrap point.

- `int uart_init(uart_t *u, bus_t *b, uint32_t timeout)`: zero `*u`, set the bus, write CTRL = RESET, wait for RESETTING to clear (bounded, as in the bounded-poll lesson; return -1 on timeout without enabling anything), acknowledge whatever error flags are set (writing exactly those bits) **without counting them**, then set RXIE leaving the other CTRL bits alone. Return 0.
- `void uart_isr(uart_t *u)`: read STATUS; count and acknowledge OVERRUN and FRAME (one count per flag per call, write exactly the observed bits); move received bytes from DATA to the receive ring while RXNE is set (at most 16 bytes per call, re-reading STATUS after each byte). If the ring is full, still read DATA but count `rx_dropped` and discard the **new** byte. Then, if TXE is set in the latest STATUS: if the transmit ring has a byte, write it to DATA; otherwise, if `u->txie`, clear TXIE (read-modify-write CTRL) and `u->txie = 0`.
- `size_t uart_write(uart_t *u, const uint8_t *p, size_t n)`: queue as many bytes as fit and return that count. If at least one was queued and `!u->txie`, set TXIE (read-modify-write, keeping RXIE) and `u->txie = 1`. With nothing queued, touch no register.
- `int uart_read(uart_t *u, uint8_t *out)`: 1 and the oldest byte, or 0 if empty. No bus access.

## What the simulation does not cover

The tests call `uart_isr` between driver calls, never in the middle of one. A real ISR can interrupt `uart_write` between updating the ring and enabling TXIE; making that safe needs a critical section (briefly masking interrupts) or careful single-writer discipline. That reasoning, and the choice of dropping the newest byte versus the oldest when the receive ring is full, are design decisions the tests can pin down only for the stated policy, not justify. Write down in a comment why your driver is correct under the model, then think about what breaks outside it.
