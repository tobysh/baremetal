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

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off) {
    return 0;
}

uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off) {
    return 0;
}

void pci_write32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off, uint32_t value) {
}

int pci_scan(PciInfo *out, int max) {
    return 0;
}

int pci_find(uint16_t vendor, uint16_t device, PciInfo *out) {
    return 0;
}

uint32_t pci_bar_size(uint8_t dev, uint8_t fn, int bar) {
    return 0;
}

// Optional: press Run to list the devices on the serial port.
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
