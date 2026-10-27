---
title: Hardware Interrupts: PIC and Timer
summary: Remap the 8259 interrupt controller, program the PIT to tick 100 times a second, acknowledge each IRQ, and build sleep on top of real timer interrupts.
minutes: 55
difficulty: medium
tags: hardware, interrupts, timer
id: osdev/pic-timer
mode: harness
---
Exceptions come from the CPU. **Interrupts** come from devices: a key is pressed, a packet arrives, a timer expires. Without them a kernel could only poll, spinning in loops asking "has anything happened yet?". With them the CPU can sleep (`hlt`) until something real occurs. In this lesson you get the first useful interrupt working: a timer tick.

## The 8259 PIC

Devices don't talk to the CPU directly. They raise a line (an **IRQ**, numbered 0 to 15) on a chip called the **Programmable Interrupt Controller**. There are two, a *master* (IRQ 0 to 7) and a *slave* (IRQ 8 to 15) wired into the master's IRQ 2. When an IRQ fires, the PIC tells the CPU a **vector number** and the CPU runs the IDT gate for that vector.

There is a problem. At power-on the master maps IRQ 0 to 7 to vectors **8 to 15**, which are already taken by CPU exceptions (8 is double fault!). So the first job of every kernel is to **remap** the PICs to vectors that are free. The usual choice is vectors 32 to 39 for the master and 40 to 47 for the slave.

### The path of one interrupt

It helps to follow a single tick from the chip to your C code:

```
PIT channel 0 output  --->  PIC master IRQ 0 line
                               |  PIC picks vector = offset + 0 = 32
                               v
CPU (if IF = 1)  <--- INT signal + vector number 32
   |  saves frame, loads IDT gate 32, jumps to its stub
   v
stub pushes registers -> calls interrupt_handler(32, 0, rip)
   |  you: ticks++, send EOI to the PIC
   v
stub pops registers, iretq -> your interrupted code carries on
```

Three separate gates must be open for this to happen. The PIT must be programmed to tick. The PIC must have IRQ 0 **unmasked**. And the CPU must have `IF` set. Because any one of them being closed produces the same symptom (nothing happens), a good debugging question is "which of the three is closed?"

### Why the remap exists

The PC's designers wired IRQ 0 to 7 to vectors 8 to 15 on the original 8086, which had no CPU exceptions in that range. Later CPUs (from the 286) defined vectors 0 to 31 for their own exceptions, and the PC could not change its wiring without breaking old software. The result is a famous clash: an IRQ 0 timer tick would arrive as vector 8, which the CPU defines as a *double fault*. Your handler could not tell the two apart. The fix is the one every operating system performs: reprogram the PIC so IRQ 0 to 7 produce vectors 32 to 39 instead. You are doing, in a few `outb`s, what every OS since the 1990s does at boot.

Remapping means sending an initialisation sequence, called ICW1 to ICW4, to each chip's command and data ports:

| Chip | Command port | Data port |
|------|--------------|-----------|
| Master | `0x20` | `0x21` |
| Slave | `0xA0` | `0xA1` |

1. **ICW1**, to the command port: `0x11` (start initialisation, expect ICW4).
2. **ICW2**, to the data port: the vector offset. 32 for the master, 40 for the slave.
3. **ICW3**, to the data port: wiring. Master: `4` (the slave is on IRQ 2, bit 2). Slave: `2` (its cascade identity).
4. **ICW4**, to the data port: `0x01` (8086 mode).

What each word means in practice:

- **ICW1 `0x11`.** Bit 4 says "this is initialisation", and bit 0 says an ICW4 will follow. Writing it puts the chip in a mode where the next three writes to the *data* port are interpreted as ICW2, ICW3 and ICW4, in that order. That is why the order cannot change, and why a stray write between them corrupts the setup.
- **ICW2.** The offset must be a multiple of 8, because the chip fills in the low three bits with the IRQ number: IRQ 3 on the master with offset 32 gives vector 35.
- **ICW3.** The master gets a bit mask of which IRQ lines have a slave (`4` is `0b100`, bit 2). The slave gets the *number* of the master line it is attached to (`2`). Same wiring, described from each side.
- **ICW4 `0x01`.** Selects 8086 mode, the only one that makes sense on a modern CPU. Without it the chip speaks a protocol for 8-bit processors.

Starting the sequence also **resets the interrupt mask**, so read both masks first and write them back afterwards. The mask register (read and write the data port) has one bit per IRQ: **1 means masked** (blocked).

## End of interrupt

After handling an IRQ you must tell the PIC, by writing `0x20` ("end of interrupt") to the command port of the chip that raised it. Until you do, that IRQ line and all lower-priority ones stay blocked. For an IRQ from the slave (8 to 15) send EOI to the **slave and then the master**, because the master also considers its IRQ 2 in service.

## Spurious IRQs

Rarely the PIC raises IRQ 7 (or 15) and then the line drops before the CPU asks. This **spurious** interrupt must not be acknowledged. To tell, write `0x0B` to the chip's command port and read the same port back: that is the *in-service register*. If bit 7 is 0, it's spurious. (For a spurious IRQ 15, the master still needs an EOI because it did see IRQ 2.)

### Why EOI is needed, and how masks work

The PIC is a state machine. When it delivers an IRQ it marks that line **in service** and will not deliver that line or any lower-priority one again until you send EOI. A forgotten EOI is why a timer works for exactly one tick and then goes silent. (IRQ 0 has the highest priority, then 1, 2 and so on, with the slave's IRQs ranking at the master's IRQ 2 position.) An EOI too many is also a bug, because it may clear a different line's in-service bit.

The mask is a plain byte per chip, so setting and clearing one IRQ is a read-modify-write of a data port:

```c
uint8_t mask = inb(port);        // current mask of that chip
mask |=  (1 << bit);             // mask (block) the IRQ
mask &= ~(1 << bit);             // unmask (allow) the IRQ
outb(port, mask);
```

For an IRQ `n` from 8 to 15 the chip is the slave (`0xA1`) and the bit is `n - 8`. Unmasking a slave IRQ also requires master IRQ 2 to be unmasked, because that is where the slave's signal comes in. Your `timer_init` masks everything except IRQ 0, so you never need that here, but a keyboard (IRQ 1) or the RTC (IRQ 8) would.

## The PIT

The **Programmable Interval Timer** is a chip that divides a fixed 1,193,182 Hz clock. Channel 0 is wired to IRQ 0. To make it tick at `hz`:

1. Write `0x36` to port `0x43` (channel 0, low byte then high byte, square wave).
2. Write the 16-bit divisor `1193182 / hz` to port `0x40`, low byte first, then high byte.

### Working out the numbers

The divisor is `1193182 / hz` with integer division. For 100 Hz that is `11931` (the exact answer, 11931.82, is truncated), so the real rate is `1193182 / 11931`, about 100.0015 Hz. Close enough for a scheduler, but you now know why a kernel's clock drifts unless it is calibrated. The divisor is 16 bits, so the slowest tick is about 18.2 Hz (a value of 0 means 65,536, the power-on default), and the fastest sensible one is a few kHz.

The command byte `0x36` is `00 11 011 0` in binary: channel 0, access mode "low byte then high byte", operating mode 3 (square wave), binary counting. After it, the *two* writes to port `0x40` are low byte first, then high byte: the same port receives both halves in sequence, as the command told it to expect. Writing only one byte leaves the divisor half-set.

The high byte is simply `divisor >> 8`, and the low byte is `divisor & 0xFF`.

## Turning interrupts on

The CPU ignores maskable interrupts until its **interrupt flag** is set. `sti` sets it, `cli` clears it. `hlt` stops the CPU until the next interrupt, so a sleep function is just "`hlt` until enough ticks have gone by."

### Sleeping without wasting the CPU

A sleep built on `hlt` looks like this:

```c
uint64_t target = ticks + n;
while (ticks < target)
    __asm__ volatile ("hlt");
```

Three details are worth understanding:

1. **`ticks` must be `volatile`.** The loop never writes it, so without `volatile` the compiler may read it once and loop forever. The interrupt handler changes it behind the compiler's back, exactly the situation `volatile` is for.
2. **`hlt` only wakes for an interrupt**, and IF must be set or it sleeps forever. That is why `timer_init` finishes with `sti`.
3. **Compute the target before the loop**, not inside it, so you wait for `n` ticks *from now*. Reading `ticks` once up front also avoids a subtle bug if a tick lands halfway through your check.

If a tick arrives just between the comparison and `hlt`, the CPU simply sleeps until the *next* tick (10 ms at 100 Hz), so the wait is never wrong, only occasionally 10 ms longer. Real kernels close that window with `sti; hlt`, since `sti` takes effect only after the instruction that follows it.

### Spurious interrupts, explained

A spurious IRQ happens when a device raises its line, the PIC tells the CPU, and the line drops before the CPU acknowledges. The PIC has to answer with *something*, so it answers with its lowest-priority IRQ (7 on the master, 15 on the slave). The handler can tell a genuine IRQ 7 from a fake by reading the in-service register: a real one has bit 7 set, a spurious one does not. You must not EOI a spurious IRQ 7 (there is nothing in service to clear). For a spurious IRQ 15 the slave needs no EOI but the master does, because the master really did accept the signal on its IRQ 2 line.

## Your turn

The starter installs an IDT for vectors 0 to 47 for you and provides `sti`/`cli` wrappers. You write the PIC and PIT code, the interrupt handler, and the sleep. You will need three ingredients to get a tick: the right vector (remap), IRQ 0 unmasked, and an EOI after every tick. If the tests report that no ticks arrived, one of them is missing.
