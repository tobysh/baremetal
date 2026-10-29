`cpuid_raw` is one `__asm__ volatile` statement with outputs `"=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3])` and inputs `"a"(leaf), "c"(sub)`. Using `r[...]` directly as outputs is fine; or use four local variables and copy them.
===
For the vendor, call `cpuid_raw(0, 0, r)` and then `memcpy(out, &r[1], 4); memcpy(out + 4, &r[3], 4); memcpy(out + 8, &r[2], 4); out[12] = 0;`. `memcpy` is declared for you in the starter.
===
`cpu_feature`: return 0 unless `reg` is `CPU_EDX` or `CPU_ECX`; call `cpuid_raw(1, 0, r)` and return `(r[reg == CPU_EDX ? 3 : 2] >> bit) & 1`. Guard `bit` to `0..31` too, because shifting by 32 or more is undefined.
===
`cpu_has_long_mode`: first `cpuid_raw(0x80000000, 0, r)`; if `r[0] < 0x80000001` return 0. Then read leaf `0x80000001` and test `r[3] & (1u << 29)`.
===
```c
uint64_t read_msr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}
int long_mode_active(void) { return (read_msr(0xC0000080) >> 10) & 1; }
uint64_t read_cr0(void) { uint64_t v; __asm__ volatile("mov %%cr0, %0" : "=r"(v)); return v; }
```
