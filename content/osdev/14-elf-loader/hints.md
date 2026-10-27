The header is a plain struct: `const Elf64_Ehdr *eh = (const Elf64_Ehdr *)image;`. Check `size < sizeof(Elf64_Ehdr)` before reading any field. The magic bytes are `eh->e_ident[0] == 0x7F && e_ident[1] == 'E' ...`.
===
Program header table check without overflow: `eh->e_phoff <= size && (uint64_t)eh->e_phnum * 56 <= size - eh->e_phoff`. Here `e_phnum` is 16 bits, so the product cannot overflow, and `size - e_phoff` cannot underflow because of the first test.
===
Get the table as `const Elf64_Phdr *ph = (const Elf64_Phdr *)(image + eh->e_phoff);`. A `for` loop over `e_phnum` entries, skipping `p_type != PT_LOAD`.
===
Do two passes. Pass 1 checks every `PT_LOAD` and notes whether the entry lies in one (`entry >= vaddr && entry - vaddr < memsz`). Pass 2, which runs only if pass 1 found no error, does the copy loops.
===
Pass 2 body: `uint8_t *dst = memory + (ph[i].p_vaddr - mem_base);` copy `p_filesz` bytes from `image + p_offset`, then set `p_memsz - p_filesz` bytes to 0 after them.
