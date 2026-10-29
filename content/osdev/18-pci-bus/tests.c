static uint32_t raw_read(int dev, int fn, int off) {
    bm_outl(0xCF8, 0x80000000u | ((uint32_t)dev << 11) | ((uint32_t)fn << 8) | (uint32_t)(off & 0xFC));
    return bm_inl(0xCFC);
}

void kmain(void) {
    CHECK("pci_read32 reads vendor and device of the host bridge", pci_read32(0, 0, 0, 0) == raw_read(0, 0, 0) && (pci_read32(0, 0, 0, 0) & 0xFFFF) == 0x8086,
          "read 0x%08x", pci_read32(0, 0, 0, 0));
    CHECK("pci_read32 on an empty slot reads all ones", pci_read32(0, 29, 0, 0) == 0xFFFFFFFFu, "read 0x%08x", pci_read32(0, 29, 0, 0));
    CHECK("pci_read32 uses the device number (bits 11-15)", pci_read32(0, 2, 0, 0) == raw_read(2, 0, 0) && pci_read32(0, 2, 0, 0) != pci_read32(0, 0, 0, 0),
          "device 2 read 0x%08x, expected 0x%08x", pci_read32(0, 2, 0, 0), raw_read(2, 0, 0));
    CHECK("pci_read32 uses the function number (bits 8-10)", pci_read32(0, 1, 3, 0) == raw_read(1, 3, 0) && pci_read32(0, 1, 3, 0) != pci_read32(0, 1, 0, 0),
          "function 3 read 0x%08x, expected 0x%08x", pci_read32(0, 1, 3, 0), raw_read(1, 3, 0));
    CHECK("pci_read32 ignores the low two bits of the offset", pci_read32(0, 0, 0, 0x03) == raw_read(0, 0, 0), "read 0x%08x", pci_read32(0, 0, 0, 0x03));

    uint32_t id = raw_read(0, 0, 0);
    CHECK("pci_read16 reads the vendor id at offset 0", pci_read16(0, 0, 0, 0) == (id & 0xFFFF), "read 0x%04x", pci_read16(0, 0, 0, 0));
    CHECK("pci_read16 reads the device id at offset 2", pci_read16(0, 0, 0, 2) == (id >> 16), "read 0x%04x, expected 0x%04x", pci_read16(0, 0, 0, 2), id >> 16);

    // Command register round trip on the VGA adapter (device 2): bit 2 is writable bus-master enable.
    uint32_t cmd_before = raw_read(2, 0, 4);
    pci_write32(0, 2, 0, 4, cmd_before | 0x04);
    uint32_t cmd_set = raw_read(2, 0, 4);
    pci_write32(0, 2, 0, 4, cmd_before);
    CHECK("pci_write32 reaches the device", (cmd_set & 0x04) != 0 && raw_read(2, 0, 4) == cmd_before, "command register was 0x%08x then 0x%08x", cmd_set, raw_read(2, 0, 4));

    PciInfo list[16];
    memset(list, 0, sizeof list);
    int n = pci_scan(list, 16);
    CHECK("pci_scan finds at least four functions", n >= 4, "found %d", n);
    CHECK("pci_scan lists the host bridge first", n >= 1 && list[0].dev == 0 && list[0].fn == 0 && list[0].bus == 0 && list[0].vendor == 0x8086 && list[0].device == 0x1237,
          "first entry is %04x:%04x at %d:%d", list[0].vendor, list[0].device, list[0].dev, list[0].fn);
    CHECK("the host bridge has class 0x06, subclass 0x00", n >= 1 && list[0].class_code == 0x06 && list[0].subclass == 0x00, "class %02x subclass %02x", list[0].class_code, list[0].subclass);

    int isa = -1, ide = -1, vga = -1, ordered = 1;
    for (int i = 0; i < n; i++) {
        if (list[i].vendor == 0x8086 && list[i].device == 0x7000) isa = i;
        if (list[i].vendor == 0x8086 && list[i].device == 0x7010) ide = i;
        if (list[i].vendor == 0x1234 && list[i].device == 0x1111) vga = i;
        if (i > 0 && (list[i].dev < list[i - 1].dev || (list[i].dev == list[i - 1].dev && list[i].fn <= list[i - 1].fn))) ordered = 0;
    }
    CHECK("pci_scan finds the ISA bridge (function 0 of device 1)", isa >= 0 && list[isa].dev == 1 && list[isa].fn == 0 && list[isa].class_code == 0x06 && list[isa].subclass == 0x01,
          "ISA bridge index %d", isa);
    CHECK("pci_scan probes the other functions of a multi-function device", ide >= 0 && list[ide].dev == 1 && list[ide].fn == 1 && list[ide].class_code == 0x01,
          "IDE function not found at 1:1");
    CHECK("pci_scan finds the VGA adapter as a display controller", vga >= 0 && list[vga].dev == 2 && list[vga].class_code == 0x03 && list[vga].subclass == 0x00,
          "VGA index %d", vga);
    CHECK("pci_scan lists functions in order of device, then function", ordered, "entries are not sorted");

    PciInfo two[2];
    memset(two, 0x5A, sizeof two);
    int m = pci_scan(two, 2);
    CHECK("pci_scan respects max", m == 2 && two[0].vendor == 0x8086 && two[1].dev >= 1, "returned %d", m);
    PciInfo none[1];
    CHECK("pci_scan with max 0 stores nothing", pci_scan(none, 0) == 0, "returned a count");

    PciInfo f;
    memset(&f, 0, sizeof f);
    CHECK("pci_find locates the VGA adapter by id", pci_find(0x1234, 0x1111, &f) == 1 && f.dev == 2 && f.fn == 0 && f.class_code == 0x03,
          "found device %d function %d", f.dev, f.fn);
    CHECK("pci_find returns 0 for a device that does not exist", pci_find(0xBEEF, 0xCAFE, &f) == 0, "reported a match");
    CHECK("pci_find finds a non-zero function", pci_find(0x8086, 0x7010, &f) == 1 && f.dev == 1 && f.fn == 1, "found %d:%d", f.dev, f.fn);

    uint32_t bar0 = raw_read(2, 0, 0x10);
    uint32_t sz = pci_bar_size(2, 0, 0);
    CHECK("pci_bar_size measures the VGA framebuffer BAR as a power of two of at least 1 MiB", sz >= 0x100000 && (sz & (sz - 1)) == 0, "size is 0x%x", sz);
    CHECK("pci_bar_size restores the BAR", raw_read(2, 0, 0x10) == bar0, "BAR was 0x%08x, now 0x%08x", bar0, raw_read(2, 0, 0x10));
    CHECK("an unused BAR has size 0", pci_bar_size(2, 0, 3) == 0 && pci_bar_size(0, 0, 0) == 0, "sizes %u and %u", pci_bar_size(2, 0, 3), pci_bar_size(0, 0, 0));
    uint32_t bar4 = raw_read(1, 1, 0x20);
    uint32_t iosz = pci_bar_size(1, 1, 4);
    CHECK("an I/O BAR is measured with its two flag bits masked (the IDE bus-master BAR is 16 bytes)", (bar4 & 1) && iosz == 16, "BAR4 is 0x%08x, size %u", bar4, iosz);
    CHECK("the I/O BAR is restored too", raw_read(1, 1, 0x20) == bar4, "BAR4 was 0x%08x, now 0x%08x", bar4, raw_read(1, 1, 0x20));
}
