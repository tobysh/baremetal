#include <stdint.h>
#include <stddef.h>

void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

typedef struct {
    uint8_t bus, dev, fn;
    uint16_t vendor, device;
    uint8_t class_code, subclass;
} PciInfo;

static uint32_t cfg_addr(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off) {
    return (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)fn << 8) | (off & 0xFC);
}

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off) {
    outl(PCI_ADDR, cfg_addr(bus, dev, fn, off));
    return inl(PCI_DATA);
}

uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off) {
    return (uint16_t)(pci_read32(bus, dev, fn, off) >> ((off & 2) * 8));
}

void pci_write32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off, uint32_t value) {
    outl(PCI_ADDR, cfg_addr(bus, dev, fn, off));
    outl(PCI_DATA, value);
}

static int probe(int dev, int fn, PciInfo *p) {
    uint32_t id = pci_read32(0, (uint8_t)dev, (uint8_t)fn, 0x00);
    if ((id & 0xFFFF) == 0xFFFF) return 0;
    uint32_t cls = pci_read32(0, (uint8_t)dev, (uint8_t)fn, 0x08);
    p->bus = 0;
    p->dev = (uint8_t)dev;
    p->fn = (uint8_t)fn;
    p->vendor = (uint16_t)(id & 0xFFFF);
    p->device = (uint16_t)(id >> 16);
    p->class_code = (uint8_t)(cls >> 24);
    p->subclass = (uint8_t)(cls >> 16);
    return 1;
}

int pci_scan(PciInfo *out, int max) {
    int n = 0;
    for (int dev = 0; dev < 32; dev++) {
        PciInfo p;
        if (!probe(dev, 0, &p)) continue;
        if (n < max) out[n++] = p;
        if (pci_read32(0, (uint8_t)dev, 0, 0x0C) & 0x00800000) {
            for (int fn = 1; fn < 8; fn++) {
                if (probe(dev, fn, &p) && n < max) out[n++] = p;
            }
        }
    }
    return n;
}

int pci_find(uint16_t vendor, uint16_t device, PciInfo *out) {
    PciInfo all[64];
    int n = pci_scan(all, 64);
    for (int i = 0; i < n; i++) {
        if (all[i].vendor == vendor && all[i].device == device) {
            *out = all[i];
            return 1;
        }
    }
    return 0;
}

uint32_t pci_bar_size(uint8_t dev, uint8_t fn, int bar) {
    uint8_t off = (uint8_t)(0x10 + 4 * bar);
    uint32_t saved = pci_read32(0, dev, fn, off);
    pci_write32(0, dev, fn, off, 0xFFFFFFFF);
    uint32_t mask = pci_read32(0, dev, fn, off);
    pci_write32(0, dev, fn, off, saved);
    if (mask == 0) return 0;
    if (mask & 1) return (~(mask & ~3u) + 1) & 0xFFFF;
    return ~(mask & ~0xFu) + 1;
}

static void put(char c) {
    uint8_t st;
    do { __asm__ volatile("inb %1, %0" : "=a"(st) : "Nd"((uint16_t)0x3FD)); } while (!(st & 0x20));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}
static void hex(unsigned v) {
    for (int i = 12; i >= 0; i -= 4) put("0123456789abcdef"[(v >> i) & 15]);
}
void kmain(void) {
    PciInfo list[32];
    int n = pci_scan(list, 32);
    for (int i = 0; i < n; i++) {
        hex(list[i].vendor); put(':'); hex(list[i].device); put('\r'); put('\n');
    }
}
