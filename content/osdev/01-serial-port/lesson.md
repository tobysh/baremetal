---
title: Talking to Hardware: The Serial Port
summary: Drive a UART with port I/O. Set the baud rate through the divisor latch, wait for the transmitter, and send text, the way every kernel prints its first message.
minutes: 30
difficulty: easy
tags: hardware, io, serial
id: osdev/serial-port
mode: harness
---
A kernel starts with nothing: no `printf`, no files, no screen library. The first thing almost every kernel gets working is the **serial port**, because it is simple and the emulator can pipe it straight to your terminal. When a kernel prints `hello` before anything else works, it is nearly always talking to a UART.

## How this course works

Your code is a real kernel. When you press **Test**, we compile it freestanding (no operating system underneath, no C library), link it with a tiny hidden boot stub, and boot it in a **QEMU virtual machine**. The stub switches the CPU into 64-bit mode and calls your `kmain`. Hidden tests then run *inside the machine* and inspect the real emulated hardware: ports, registers, tables and memory. If you build a wrong descriptor or forget a bit, the virtual CPU treats it exactly like a physical one would.

Your serial output appears in the output panel below the editor. A few things to know:

- There is no `malloc`, `memcpy` (use your own loops) or `printf`.
- A crash is a CPU exception. You'll see which one and at what address.
- If the machine resets with no message, that's a **triple fault**: usually a bad table.

## Port I/O

x86 has a second address space just for devices, called **I/O ports**. Two instructions reach it:

```c
void outb(uint16_t port, uint8_t value);   // write a byte to a port
uint8_t inb(uint16_t port);                // read a byte from a port
```

These compile to one instruction each (`out dx, al` and `in al, dx`). We provide them for you in this lesson. Later you'll write such wrappers yourself with inline assembly.

### Why ports exist at all

A normal `mov` reaches **memory**. The original 8086 designers also gave the CPU a second, separate address space of 65,536 **I/O ports** for talking to chips, so device registers would not use up precious memory addresses. Each port is an 8-, 16- or 32-bit register inside some chip, and reading or writing it is how you give the chip orders. Writing to a port is rarely "store a value". It is usually "do this now": send a byte, reset a flag, start a transfer. That is why the compiler must never reorder or drop these accesses, and why the real versions of `outb` and `inb` use inline assembly the optimiser cannot see through.

Ports are also privileged. A normal user program is not allowed to run `in` or `out`, which is one reason the operating system exists: it is the only code trusted to touch hardware directly.

## What a UART does

**UART** stands for Universal Asynchronous Receiver/Transmitter. It converts bytes into a stream of electrical pulses on a single wire and back again. "Asynchronous" means there is no shared clock between the two ends, so both agree beforehand on how fast to send (the **baud rate**) and what a frame looks like. A single byte on the wire is one **frame**:

```
idle  start   d0 d1 d2 d3 d4 d5 d6 d7   stop   idle
 1      0     <-- 8 data bits, low first -->  1     1
```

The line rests at 1. A 0 for one bit time (the **start bit**) wakes the receiver, then eight data bits follow, least significant first, then a 1 (the **stop bit**) puts the line back to idle. That is the "8N1" in the setup steps: **8** data bits, **N**o parity, **1** stop bit. It costs 10 bits on the wire per byte, so at 38400 baud you move about 3840 bytes per second. A UART is slow compared with a CPU that executes billions of instructions a second, which is the real reason you must wait before sending the next byte.

## The 16550 UART

The first serial port, **COM1**, lives at port `0x3F8`. It has eight registers at consecutive port numbers:

| Port | Read | Write |
|------|------|-------|
| `base + 0` | received byte | byte to transmit (or divisor low byte when DLAB = 1) |
| `base + 1` | | interrupt enable (or divisor high byte when DLAB = 1) |
| `base + 2` | | FIFO control |
| `base + 3` | | line control |
| `base + 4` | | modem control |
| `base + 5` | line status | |

Registers 0 and 1 do double duty. Bit 7 of the line control register is the **DLAB** (divisor latch access bit). While it is 1, registers 0 and 1 hold a 16-bit **divisor** that sets the speed: baud = 115200 / divisor. While it is 0, they are the data and interrupt-enable registers.

## Initialising it

1. Disable UART interrupts: write `0x00` to `base + 1`.
2. Set DLAB: write `0x80` to `base + 3`.
3. Write the divisor, low byte to `base + 0`, high byte to `base + 1`. Divisor 3 gives 38400 baud.
4. Clear DLAB and choose the frame format **8N1** (8 data bits, no parity, 1 stop bit): write `0x03` to `base + 3`.
5. Enable and clear the FIFOs: write `0xC7` to `base + 2`.
6. Raise DTR and RTS: write `0x03` to `base + 4`.

The order matters. You have to open the latch before you can write the divisor, and close it before you can send data.

### Reading each value

Where the numbers come from is worth understanding, because every driver you write later is built the same way: read the datasheet, then turn each field into bits.

- **Divisor.** The UART has a base clock of 115,200 ticks a second. Dividing it by `divisor` gives the baud rate: `115200 / 3 = 38400`. Divisor 1 is 115,200 baud, 12 is 9600. The divisor is 16 bits, which is why it needs two registers (low byte, high byte) behind the DLAB switch. For 3, the low byte is `0x03` and the high byte `0x00`.
- **Line control `0x03`.** Bits 0 and 1 choose the word length (`11` means 8 bits). Bit 2 chooses 1 stop bit when 0. Bits 3 to 5 choose parity, and `000` means none. Bit 7 is DLAB and is 0, which is how this one write both sets the format and closes the latch.
- **FIFO control `0xC7`.** Bit 0 enables the FIFOs (a 16-byte queue so the CPU can hand over several bytes at once), bits 1 and 2 clear the receive and transmit queues, and bits 6 and 7 set when the chip would raise an interrupt. You do not use interrupts yet.
- **Modem control `0x03`.** Bits 0 and 1 are DTR (data terminal ready) and RTS (request to send), the flow-control lines that tell the other end you are listening.

### Why the order matters

Registers 0 and 1 mean different things depending on DLAB, so the sequence is really a small state machine:

```
interrupts off -> DLAB on -> write divisor -> DLAB off + format -> FIFO -> modem
```

If you write the divisor with DLAB still off, you would send a byte and set the interrupt enable register instead. Nothing crashes. The port simply produces the wrong speed or none at all, and with hardware you cannot see into, that kind of silent failure is the main reason to follow the manual's order exactly.

## Sending a byte

The transmitter is slow compared to the CPU. If you write a second byte before the first has left, the first is lost. Bit 5 of the line status register (`0x20`, "transmit holding register empty") says when it is safe. So sending is a **polling loop**:

```c
while ((inb(COM1 + 5) & 0x20) == 0) { /* wait */ }
outb(COM1, byte);
```

### Polling and its cost

The loop above is called **busy-waiting** or **polling**: the CPU does nothing useful while it spins. For a serial port at boot that is fine and keeps the code simple. In a finished kernel you would instead let the UART raise an **interrupt** when it can accept more data, and spend the waiting time running other work. You will build exactly that machinery in the interrupt lessons, and serial drivers are a common first user of it.

The line status register has other useful bits you are not using yet: bit 0 means a received byte is waiting to be read, and bit 6 means the transmitter has finished with *everything*, including the byte currently being shifted out. Bit 5 is enough for sending because it says the holding register is free for the next byte.

## Newlines

A terminal wants `\r\n` (carriage return, then line feed). C strings only have `\n`. Your `serial_puts` should turn every `\n` into `\r\n`.

## Your turn

Implement `serial_init`, `serial_putc` and `serial_puts`. You can also write a `kmain` that calls them and press **Run** to see your text appear in the output panel.
