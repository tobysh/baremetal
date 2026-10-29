Implement these functions. The hidden tests compare them with the real CPU running your kernel.

- `void cpuid_raw(uint32_t leaf, uint32_t sub, uint32_t r[4])` runs `cpuid` and stores `eax`, `ebx`, `ecx`, `edx` in `r[0]` to `r[3]`.
- `void cpu_vendor(char out[13])` writes the 12-character vendor string followed by a `'\0'`.
- `uint32_t cpu_max_leaf(void)` returns the highest basic leaf (the `eax` of leaf 0).
- `int cpu_feature(int reg, int bit)` returns 1 if bit `bit` of leaf 1 is set. `reg` is `CPU_EDX` (0) or `CPU_ECX` (1), and otherwise the function returns 0.
- `int cpu_has_long_mode(void)` returns 1 if the extended leaf `0x80000001` reports long mode (`edx` bit 29). It returns 0 if the CPU has no extended leaves that high.
- `uint64_t read_cr0(void)` and `uint64_t read_cr4(void)` return the control registers.
- `uint64_t read_msr(uint32_t msr)` returns the MSR as one 64-bit value.
- `int long_mode_active(void)` returns 1 when EFER's LMA bit is set.
