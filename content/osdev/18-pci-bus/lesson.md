---
title: Finding Devices on the PCI Bus
summary: Read PCI configuration space through ports 0xCF8 and 0xCFC, enumerate the devices on bus 0 including multi-function ones, and measure a BAR by writing all ones.
minutes: 50
difficulty: medium
tags: hardware, io, pci
id: osdev/pci-bus
mode: harness
---
The serial port and VGA text memory live at addresses everybody knows. Almost everything else in a modern PC (disk controllers, network cards, graphics) sits on the **PCI bus**, and a kernel has to *discover* it: there is no fixed list. Each device answers questions about itself in a small block of registers called its **configuration space**, and a kernel enumerates the bus to find out what is plugged in.

## Addressing a device

A PCI function is named by three numbers: **bus** (0 to 255), **device** (0 to 31) and **function** (0 to 7). Together with a register **offset** into the 256-byte configuration space they make one 32-bit address, which you write to port `0xCF8`; the register's value then appears at port `0xCFC`:

```
bit  31     30..24    23..16   15..11    10..8      7..2        1..0
     enable  reserved  bus      device    function   register    0
```

```c
uint32_t addr = (1u << 31) | (bus << 16) | (dev << 11) | (fn << 8) | (off & 0xFC);
outl(0xCF8, addr);
uint32_t value = inl(0xCFC);
```

Reads are always 32 bits aligned to 4 bytes, so the low two bits of the offset are cleared in the address. To read a 16-bit field at offset 2, read the dword at offset 0 and shift right by 16 (`(off & 2) * 8` bits).

## The header

Every function starts with the same header:

| Offset | Size | Field |
|---|---|---|
| `0x00` | 16 | **Vendor ID** (`0xFFFF` means "no device here") |
| `0x02` | 16 | Device ID |
| `0x04` | 16 | Command (bit 0 I/O decode, bit 1 memory decode, bit 2 bus master) |
| `0x08` | 8 | Revision |
| `0x09`..`0x0B` | 24 | Programming interface, **subclass**, **class** |
| `0x0E` | 8 | Header type (bit 7 set = the device has several functions) |
| `0x10`..`0x24` | 6 x 32 | **BARs** (base address registers) |

A class of `0x03` is a display controller, `0x02` a network controller, `0x01` mass storage, `0x06` a bridge.

## Enumerating

The simple scan looks at bus 0, devices 0 to 31. For each, read function 0's vendor ID. If it is `0xFFFF` nothing is there, skip the whole slot. Otherwise record it, and if bit 7 of the header type is set the device is **multi-function**, so also probe functions 1 to 7 (each may or may not exist; again, `0xFFFF` means absent). Real kernels also follow PCI-to-PCI bridges to other buses; we stay on bus 0.

## BARs and sizing

A BAR tells the kernel where a device's registers or memory live, but first you need to know *how big* the region is. The trick: write all ones to the BAR, read it back, and the bits the device doesn't implement read as zero. For a memory BAR (bit 0 clear) mask off the low four flag bits, invert and add one:

```c
uint32_t saved = read(bar);
write(bar, 0xFFFFFFFF);
uint32_t mask = read(bar);
write(bar, saved);                          // put it back!
size = ~(mask & ~0xF) + 1;                  // memory BAR
```

For an I/O BAR (bit 0 set) the flag bits are the low two, so mask with `~0x3` and compute the size in 32 bits (the upper half of the mask is not meaningful there; use `(~(mask & ~0x3u) + 1) & 0xFFFF`).

While you do this the device briefly decodes a nonsense address, so a real kernel switches decoding off in the command register first. We keep it simple and restore the value straight away.

## Debugging checklist

- **Every read returns `0xFFFFFFFF`:** bit 31 of the address (the enable bit) is missing, or `outl` was used with the wrong port.
- **A device is found at the wrong place:** device goes in bits 11 to 15 (`<< 11`) and function in bits 8 to 10 (`<< 8`).
- **The multi-function sibling is missing:** you only probe functions 1 to 7 when bit 7 of the header type is set.
- **A BAR shows a different value afterwards:** you forgot to write the saved value back.

## Your turn

The hidden tests read the real configuration space of the simulated machine: the host bridge, the ISA bridge (a multi-function device) and the VGA adapter.
