The `outl` and `inl` port helpers and the `PciInfo` struct are in the starter. Implement (everything on bus 0 unless an argument says otherwise):

- `uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)` reads a 32-bit register (the low two bits of `off` are ignored).
- `uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)` reads a 16-bit register at an offset that is a multiple of 2.
- `void pci_write32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off, uint32_t value)` writes a 32-bit register.
- `int pci_scan(PciInfo *out, int max)` lists every function present on bus 0 in order of device, then function, storing `bus`, `dev`, `fn`, `vendor`, `device`, `class_code` and `subclass`. Functions 1 to 7 are probed only on multi-function devices. At most `max` entries are stored; the return value is the number stored.
- `int pci_find(uint16_t vendor, uint16_t device, PciInfo *out)` finds the first matching function (same order as `pci_scan`), fills `*out` and returns 1, or returns 0 if there is none.
- `uint32_t pci_bar_size(uint8_t dev, uint8_t fn, int bar)` returns the size in bytes of BAR number `bar` (0 to 5) of a function on bus 0, or 0 if the BAR is not implemented (it reads back as 0 after writing ones). The original BAR value must be restored.
