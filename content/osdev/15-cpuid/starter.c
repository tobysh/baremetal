#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);

#define CPU_EDX 0
#define CPU_ECX 1

void cpuid_raw(uint32_t leaf, uint32_t sub, uint32_t r[4]) {
}

void cpu_vendor(char out[13]) {
}

uint32_t cpu_max_leaf(void) {
    return 0;
}

int cpu_feature(int reg, int bit) {
    return 0;
}

int cpu_has_long_mode(void) {
    return 0;
}

uint64_t read_cr0(void) {
    return 0;
}

uint64_t read_cr4(void) {
    return 0;
}

uint64_t read_msr(uint32_t msr) {
    return 0;
}

int long_mode_active(void) {
    return 0;
}

// Optional: press Run to see what your kernel learns about the CPU on the serial port.
void outb(uint16_t port, uint8_t value);
static void say(const char *s) {
    for (; *s; s++) {
        while (1) {
            uint8_t st;
            __asm__ volatile("inb %1, %0" : "=a"(st) : "Nd"((uint16_t)0x3FD));
            if (st & 0x20) break;
        }
        outb(0x3F8, (uint8_t)*s);
    }
}
void kmain(void) {
    char v[13];
    cpu_vendor(v);
    say("vendor: ");
    say(v);
    say("\r\n");
}
