---
title: Talking to Hardware: The Serial Port
summary: Drive a UART with port I/O. Set the baud rate through the divisor latch, wait for the transmitter, and send text, the way every kernel prints its first message.
minutes: 30
difficulty: easy
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

## Sending a byte

The transmitter is slow compared to the CPU. If you write a second byte before the first has left, the first is lost. Bit 5 of the line status register (`0x20`, "transmit holding register empty") says when it is safe. So sending is a **polling loop**:

```c
while ((inb(COM1 + 5) & 0x20) == 0) { /* wait */ }
outb(COM1, byte);
```

## Newlines

A terminal wants `\r\n` (carriage return, then line feed). C strings only have `\n`. Your `serial_puts` should turn every `\n` into `\r\n`.

## Your turn

Implement `serial_init`, `serial_putc` and `serial_puts`. You can also write a `kmain` that calls them and press **Run** to see your text appear in the output panel.
