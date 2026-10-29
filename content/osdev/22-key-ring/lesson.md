---
title: Typing Without Losing Keys: The IRQ Ring Buffer
summary: Hand keystrokes from an interrupt handler to the main loop through a lock-free ring buffer, and sleep safely with cli, a check and sti; hlt until a key arrives.
minutes: 55
difficulty: medium
tags: interrupts, input, data-structures
id: osdev/key-ring
mode: harness
---
When you press a key the keyboard controller raises **IRQ 1**. Your handler runs at once, but the code that wants the character (a shell, a text editor) is somewhere else, doing something else. The handler cannot call it directly: handlers must be short, and the consumer may not even be ready. So the handler **puts the byte in a queue** and returns. The consumer takes it out whenever it likes. The data structure for this is the **ring buffer**.

## The ring

A fixed array plus two counters:

```c
#define RING_SIZE 16               // a power of two

typedef struct {
    uint8_t data[RING_SIZE];
    uint32_t head;                 // total bytes ever pushed
    uint32_t tail;                 // total bytes ever popped
} Ring;
```

The trick that makes this clean is to let `head` and `tail` **count forever** and only reduce them when indexing:

- bytes in the ring: `head - tail` (unsigned subtraction is correct even after the counters wrap past 2^32);
- empty: `head == tail`; full: `head - tail == RING_SIZE`;
- push: `data[head % RING_SIZE] = b; head++`;
- pop: `b = data[tail % RING_SIZE]; tail++`.

Because `RING_SIZE` is a power of two, `% RING_SIZE` is `& (RING_SIZE - 1)`. There is no "one slot always empty" trick and no special wrap-around code. For a keyboard, when the ring is full the new byte is **dropped** (`ring_push` returns 0): a stalled consumer must not corrupt old keystrokes or block an interrupt handler.

## One producer, one consumer

The handler only moves `head` (push); the main code only moves `tail` (pop). Neither writes the other's counter, which is why this queue needs no lock for a single producer and a single consumer, as long as the counters are updated *after* the data is stored and loads are not reordered by the compiler. Mark the counters `volatile`.

But there is still one hazard: `ring_pop` reads `head` and `tail`, and an interrupt could push in the middle. That's fine for the ring itself. What is not fine is a **check-then-sleep** sequence in the consumer:

```c
if (ring_count(&r) == 0) hlt;     // BUG: a key may arrive between the test and hlt
```

The solution from the previous lesson: disable interrupts, test the ring, and if it is empty execute `sti; hlt`. The `sti` takes effect after the next instruction, so the interrupt can arrive only once the CPU is already halted, which wakes it.

```c
for (;;) {
    cli;
    if (ring_pop(&r, &b)) { sti; return b; }
    sti; hlt;                      // atomic pair
}
```

## The handler

Your IRQ 1 handler drains the controller: while status port `0x64` bit 0 is set, read port `0x60` and push the byte (a byte whose status bit 5 is set came from the mouse, so discard it). Then it must send the **end of interrupt** for IRQ 1, or the PIC will never deliver another keyboard interrupt. The starter provides the IDT, PIC and PIT code from the earlier lesson, so you can concentrate on the data flow.

## Debugging checklist

- **After the 16th key nothing is stored but the program is fine:** that's the policy; make sure `ring_push` returns 0 and does not move `head`.
- **Keys work once:** the handler forgot the EOI.
- **Random keys vanish under load:** the handler wrote to `tail`, or you used a size that is not a power of two together with `& (RING_SIZE - 1)`.
- **The kernel sometimes sleeps through a key:** the consumer tested the ring with interrupts enabled before `hlt`.

## Your turn

The tests inject real keystrokes through the controller, so the keys travel the whole path: controller, PIC, CPU, IDT, your handler, your ring.
