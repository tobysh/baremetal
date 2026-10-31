---
title: A Flash Controller State Machine
summary: Unlock a write-protected flash controller once, program words with busy waits, and retry transient errors without retrying permanent ones.
minutes: 35
difficulty: hard
tags: embedded, state-machine, error-handling
id: c-embedded/flash-state-machine
mode: harness
---
Many peripherals are **state machines** you must drive in the right order. Writing the right values in the wrong state is silently ignored, or worse.

## The controller

| offset | name | meaning |
|---|---|---|
| 0x00 | CMD | write 1 to start PROGRAM |
| 0x04 | ADDR | byte address, word aligned |
| 0x08 | DATA | word to program |
| 0x0C | STATUS | bit 0 BUSY, bit 1 ERR (write-one-to-clear), bit 2 LOCKED |
| 0x10 | KEY | unlock sequence |

**Lock state machine.** The controller powers up LOCKED. Writing `0x45670123` then `0xCDEF89AB` to KEY unlocks it. Any other value written to KEY while half-unlocked, or *any* KEY write while already unlocked, makes the lock **sticky**: only a reset clears it, and every program fails. So the driver must only write the keys when STATUS says LOCKED, and never twice.

**Programming.** While BUSY, writes to ADDR, DATA and CMD are silently dropped. A CMD write is also dropped while ERR is set. A program takes several ticks, then either completes or sets ERR. Flash can only be written once: a word is programmed only if it still reads `0xFFFFFFFF` (blank). Addresses at or above `0x60` are write-protected and always fail.

## The driver

`wait_not_busy(b, timeout)` is provided (it is the bounded poll from the earlier lesson: 0 or -1; each wait gets its own budget). You write:

- `int flash_unlock(bus_t *b)` returns 0 if unlocked afterwards (writing the keys only if LOCKED is set), or -4 if it is still locked after the sequence (sticky).
- `int flash_program(bus_t *b, uint32_t addr, uint32_t data, uint32_t timeout)`:
  1. `flash_unlock`; return -4 if it fails.
  2. Up to **3 attempts**: wait until not busy; write ADDR, DATA, then CMD = 1; wait until not busy; read STATUS once. No ERR: return 0. ERR: acknowledge it by writing exactly the ERR bit, then try again.
  3. Three failed attempts: return -3.
  4. If either wait times out, return -1 immediately. A busy controller is not retried.

Retrying is right for a transient fault and pointless for a permanent one (protected address, non-blank word). The driver cannot tell them apart, so it stops after a fixed number of tries. **Limitation:** a real controller also needs an erase step, supply checks and verify-after-write. The model keeps only what this lesson teaches. Choosing "3" is a design decision that tests cannot justify for you: think about what a longer retry loop would cost on real silicon.
