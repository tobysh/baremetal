---
title: Loading a Program: The ELF Loader
summary: Parse and validate an ELF64 executable, load its segments into memory with the bss zeroed, reject malformed files safely, and jump into the program you loaded.
minutes: 60
difficulty: hard
tags: elf, processes, loader
id: osdev/elf-loader
mode: harness
---
Your kernel can print, handle interrupts, manage memory, schedule and serve system calls. But it only runs the code compiled into it. To be an *operating system* it must load **other programs**. On Linux and most other systems, a compiled program is an **ELF** file (Executable and Linkable Format). In this last lesson you write the part of the kernel that reads one and brings it to life.

## What an ELF file contains

Every ELF64 file starts with a 64-byte **header**:

| Offset | Size | Field | What we need |
|-------:|-----:|-------|--------------|
| 0 | 4 | `e_ident[0..3]` | magic: `0x7F 'E' 'L' 'F'` |
| 4 | 1 | `e_ident[EI_CLASS]` | `2` = 64-bit |
| 5 | 1 | `e_ident[EI_DATA]` | `1` = little endian |
| 16 | 2 | `e_type` | `2` = executable |
| 18 | 2 | `e_machine` | `62` = x86-64 |
| 24 | 8 | `e_entry` | virtual address of the first instruction |
| 32 | 8 | `e_phoff` | file offset of the program header table |
| 54 | 2 | `e_phentsize` | size of one program header (56) |
| 56 | 2 | `e_phnum` | how many program headers |

The **program headers** (each 56 bytes) tell a loader what to put where. Only entries of type `PT_LOAD` (1) matter:

| Offset | Size | Field | Meaning |
|-------:|-----:|-------|---------|
| 0 | 4 | `p_type` | 1 = load this segment |
| 8 | 8 | `p_offset` | where its bytes start in the file |
| 16 | 8 | `p_vaddr` | where they belong in memory |
| 32 | 8 | `p_filesz` | how many bytes come from the file |
| 40 | 8 | `p_memsz` | how large the segment is in memory |

Loading a `PT_LOAD` segment means: copy `filesz` bytes from `offset` to `vaddr`, then **zero the remaining `memsz - filesz` bytes**. That zero tail is the program's **bss** (uninitialised globals, which C promises start at 0). Other segment types (notes, stack hints) are skipped.

The starter defines both structures for you.

## Never trust the file

An executable is *input from outside*. A hostile or corrupt file must never be able to make the kernel write outside the memory it was given or read outside the file. Check, in this order of strictness:

1. The file is at least 64 bytes, has the magic, is 64-bit little-endian, type executable, machine x86-64.
2. `e_phentsize` is 56, and the whole program header table lies inside the file. Compute `phoff + phnum * 56` **without overflow**: `phoff` is 64-bit and attacker-controlled.
3. For each `PT_LOAD`: `filesz <= memsz`; `offset + filesz` lies inside the file (again, no overflow); and the memory range `[vaddr, vaddr + memsz)` lies inside the memory window you were given, `[mem_base, mem_base + mem_size)` (again, no overflow).
4. The entry point lies inside one of the loaded segments.

Do all the checks for **every** segment first, and only then start copying. A loader that fails halfway leaves a half-written, possibly dangerous image behind.

Safe range arithmetic: instead of `a + b <= limit`, write `a <= limit && b <= limit - a`, as in the syscall lesson.

## Running it

Once loaded, the program is just bytes in memory at the addresses it expects. The loader returns the entry address; calling it is a function pointer cast:

```c
int (*program)(void) = (int (*)(void))(uintptr_t)entry;
int result = program();
```

The tests do this with a tiny program written in raw machine code, which reads a value from its data segment and adds a bss variable that only works if you zeroed it.

## A worked example: reading a real header

These are the first 64 bytes of the image the tests build, with each field marked:

```
00: 7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00   e_ident: magic "\x7fELF", class 2 (64-bit),
                                                         data 1 (little endian), version 1, padding
10: 02 00 3e 00 01 00 00 00  <entry, 8 bytes>          e_type = 2 (executable), e_machine = 0x3e (62),
                                                         e_version = 1, e_entry
20: 40 00 00 00 00 00 00 00  <shoff, 8 bytes>          e_phoff = 0x40 (program headers start right
                                                         after this header), e_shoff
30: 00 00 00 00 40 00 38 00  02 00 00 00 00 00 00 00   e_flags, e_ehsize = 64, e_phentsize = 0x38 (56),
                                                         e_phnum = 2, section header fields
```

Every number is little endian, so `3e 00` is the 16-bit value `0x003e`. The easiest way to see these for yourself on a real binary is `readelf -h` and `readelf -l`; the program headers shown by `-l` are exactly the `PT_LOAD` entries your loader reads.

## What loading looks like in memory

The test image has two loadable segments. The first is code (`filesz == memsz`). The second is data followed by bss, which is why `memsz` is larger:

```
file:                                  memory window (mem_base = BASE):
 0x100 +------------+ code  (n bytes)    BASE        +------------+ code
       |            |                                 |            |
 0x200 +------------+ data (8 bytes)     BASE+0x1000 +------------+ data: the value 1234
       | 1234       |                                 | 1234       |
       +------------+                                 | 0 0 0 ...  | <- bss: memsz 0x20, filesz 8,
                                                      +------------+    so 0x18 bytes are zeroed
```

The loader copies 8 bytes for the data segment and then writes zeros for the next 24. Those zeros are not in the file at all. This is how a program with a 100 MB uninitialised array can still be a 10 KB executable.

## Why every check is there

Each check closes a way for a crafted file to harm the kernel:

- **Bad magic, class, type and machine:** stop the loader from treating a PNG or a 32-bit program as x86-64 code.
- **Program header table inside the file:** otherwise `ph[i]` reads kernel memory beyond the image, and what you read is then copied into the program.
- **`filesz <= memsz`:** otherwise `memsz - filesz` underflows to a huge unsigned number and the zeroing loop wipes out the kernel.
- **Segment inside the memory window:** otherwise the program chooses where in physical memory the kernel writes its bytes.
- **Entry inside a segment:** otherwise the first instruction is wherever the file says, possibly in kernel code.
- **Validate everything, then load:** a failure halfway must not leave half a program behind.

Every comparison is written to avoid wrapping. Whenever you'd write `a + b <= limit`, write `a <= limit && b <= limit - a`.

## What a full loader adds

Real operating systems do much more than this lesson: they create a **new page table** for the process and map each segment with permissions from `p_flags` (read, write and execute separately, which is where NX earns its keep), set up the user stack with `argv` and `environ`, handle `PT_INTERP` (dynamic linking: the file names another program to load first), apply relocations for position-independent executables (`ET_DYN`), and finally switch to ring 3 with `iretq` or `sysret`. You've built every ingredient except the privilege switch: the page-table mapping, the frame allocator, the heap, the scheduler and the system calls the program will use.

## Debugging checklist

- **Everything fails with `ELF_E_SMALL`:** you compare `size` against something other than the 64-byte header size, or you check after reading fields.
- **`ELF_E_PHDR` on a good file:** the entry size is `56`, and the table end is `e_phoff + e_phnum * 56`. Check it against `size` using subtraction.
- **The program loads, but returns the wrong number:** bss wasn't zeroed (the tests fill memory with `0xAA` first), or you copied `memsz` bytes from the file.
- **Memory is modified even though the load failed:** you copied inside the first loop. Do all validation in one pass and copying in a second.
- **The program crashes on its first instruction:** the entry was stored as a file offset rather than the virtual address, or the segment was copied to `memory + vaddr` instead of `memory + (vaddr - mem_base)`.

## Your turn

Implement `elf_validate` (checks 1 and 2) and `elf_load` (everything). The error codes are in the starter.
