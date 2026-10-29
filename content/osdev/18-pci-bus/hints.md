`pci_read32`: build the address from `(uint32_t)` casts of every argument (`bus` shifted by 16 would overflow a `uint8_t` if you forget the cast), `outl(0xCF8, addr)`, then `return inl(0xCFC)`. `pci_write32` is the same with `outl(0xCFC, value)` at the end.
===
`pci_read16(...)`: `return (uint16_t)(pci_read32(bus, dev, fn, off) >> ((off & 2) * 8));`.
===
For `pci_scan`, keep a small helper `static int probe(int dev, int fn, PciInfo *p)` that reads the ID dword, returns 0 if the vendor is `0xFFFF`, and fills `*p` otherwise (class at offset `0x0B`, subclass at `0x0A`, both from `pci_read32(0, dev, fn, 0x08) >> 24` and `>> 16 & 0xFF`). The loop is `for dev in 0..31 { if probe(dev,0) { record; if header_type & 0x80 { for fn in 1..7 ... } } }`.
===
`pci_find` can reuse `pci_scan` with a local array of, say, 64 entries, then search it. Check `max` before every store in `pci_scan`.
===
`pci_bar_size`: `off = 0x10 + 4 * bar`; save the value; write `0xFFFFFFFF`; read the mask; write the saved value back; if `mask == 0` return 0 (or `mask == 0xFFFFFFFF` is also not meaningful, but you may ignore that). Then `if (mask & 1) mask &= ~3u, size = (~mask + 1) & 0xFFFF; else mask &= ~0xFu, size = ~mask + 1;`.
