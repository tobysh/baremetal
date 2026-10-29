---
title: Asking the CPU Who It Is
summary: Run cpuid for the vendor and feature bits, read control registers and the EFER MSR, and prove from inside the kernel that the CPU is really in 64-bit long mode.
minutes: 30
difficulty: easy
tags: cpu, registers, cpuid
id: osdev/cpuid
mode: harness
---
Before a kernel uses a feature (paging, 1 GiB pages, a faster system-call instruction, an APIC) it has to find out whether the CPU has it. The CPU answers through one special instruction, `cpuid`, and through a handful of registers only the kernel may touch.

## `cpuid`

`cpuid` takes a **leaf** number in `eax` (and sometimes a **sub-leaf** in `ecx`), and answers by overwriting `eax`, `ebx`, `ecx` and `edx`. All four registers change, so the compiler must be told they are outputs.

```c
uint32_t a, b, c, d;
__asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(sub));
```

A few leaves worth knowing:

| Leaf | Answer |
|---|---|
| `0` | `eax` = highest basic leaf; `ebx`, `edx`, `ecx` (in that order!) spell a 12-letter **vendor string** such as `GenuineIntel` or `AuthenticAMD` |
| `1` | Feature flags: `edx` bit 0 FPU, bit 4 TSC, bit 6 PAE, bit 9 on-chip APIC; `ecx` bit 0 SSE3 |
| `0x80000000` | Highest *extended* leaf |
| `0x80000001` | `edx` bit 29 = **long mode** (64-bit) supported, bit 20 = NX bit |

The vendor string is four bytes from each register, in the register's little-endian byte order. Copy the registers into a `char` buffer with `memcpy`, in the order `ebx`, `edx`, `ecx`.

## Control registers and MSRs

- `CR0` bit 0 (**PE**) is protected mode, bit 31 (**PG**) turns paging on.
- `CR4` bit 5 (**PAE**) is physical address extension, which long mode requires.
- **Model-specific registers** (MSRs) are read with `rdmsr` (number in `ecx`, result in `edx:eax`). `EFER` is MSR `0xC0000080`: bit 8 (**LME**) is "long mode enable", bit 10 (**LMA**) is "long mode active". The CPU sets LMA itself once paging and LME are both on.

`rdmsr` returns the 64-bit value split across two registers, so you have to combine them: `((uint64_t)edx << 32) | eax`.

## Debugging checklist

- **Vendor string is garbled:** the order is `ebx`, `edx`, `ecx`, not `ebx`, `ecx`, `edx`.
- **Random crashes after `cpuid`:** you forgot that `cpuid` overwrites `ebx` as well. Declare all four registers as outputs.
- **`rdmsr` result is wrong:** combine the halves with a shift and an OR, and cast to `uint64_t` before shifting.

## Your turn

You write the `cpuid` wrapper first. Everything else is built on top of it.
