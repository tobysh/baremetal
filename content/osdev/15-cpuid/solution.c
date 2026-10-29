#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);

#define CPU_EDX 0
#define CPU_ECX 1

void cpuid_raw(uint32_t leaf, uint32_t sub, uint32_t r[4]) {
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(sub));
    r[0] = a; r[1] = b; r[2] = c; r[3] = d;
}

void cpu_vendor(char out[13]) {
    uint32_t r[4];
    cpuid_raw(0, 0, r);
    memcpy(out, &r[1], 4);
    memcpy(out + 4, &r[3], 4);
    memcpy(out + 8, &r[2], 4);
    out[12] = 0;
}

uint32_t cpu_max_leaf(void) {
    uint32_t r[4];
    cpuid_raw(0, 0, r);
    return r[0];
}

int cpu_feature(int reg, int bit) {
    if ((reg != CPU_EDX && reg != CPU_ECX) || bit < 0 || bit > 31) return 0;
    uint32_t r[4];
    cpuid_raw(1, 0, r);
    return (r[reg == CPU_EDX ? 3 : 2] >> bit) & 1;
}

int cpu_has_long_mode(void) {
    uint32_t r[4];
    cpuid_raw(0x80000000, 0, r);
    if (r[0] < 0x80000001) return 0;
    cpuid_raw(0x80000001, 0, r);
    return (r[3] >> 29) & 1;
}

uint64_t read_cr0(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr0, %0" : "=r"(v));
    return v;
}

uint64_t read_cr4(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr4, %0" : "=r"(v));
    return v;
}

uint64_t read_msr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

int long_mode_active(void) {
    return (int)((read_msr(0xC0000080) >> 10) & 1);
}

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
