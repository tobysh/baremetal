#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

#define PT_LOAD 1

#define ELF_OK          0
#define ELF_E_SMALL    -1   // file shorter than an ELF header
#define ELF_E_MAGIC    -2   // not an ELF file
#define ELF_E_CLASS    -3   // not 64-bit little endian
#define ELF_E_TYPE     -4   // not an executable (ET_EXEC = 2)
#define ELF_E_MACHINE  -5   // not x86-64 (62)
#define ELF_E_PHDR     -6   // bad program header table
#define ELF_E_SEGMENT  -7   // a segment's file data is out of bounds, or filesz > memsz
#define ELF_E_RANGE    -8   // a segment does not fit in the memory window
#define ELF_E_ENTRY    -9   // the entry point is not inside a loaded segment

int elf_validate(const uint8_t *image, size_t size) {
    if (size < sizeof(Elf64_Ehdr)) return ELF_E_SMALL;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)image;
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') return ELF_E_MAGIC;
    if (eh->e_ident[4] != 2 || eh->e_ident[5] != 1) return ELF_E_CLASS;
    if (eh->e_type != 2) return ELF_E_TYPE;
    if (eh->e_machine != 62) return ELF_E_MACHINE;
    if (eh->e_phentsize != sizeof(Elf64_Phdr)) return ELF_E_PHDR;
    if (eh->e_phoff > size || (uint64_t)eh->e_phnum * sizeof(Elf64_Phdr) > size - eh->e_phoff) return ELF_E_PHDR;
    return ELF_OK;
}

int elf_load(const uint8_t *image, size_t size, uint8_t *memory, size_t mem_size, uint64_t mem_base, uint64_t *entry) {
    int err = elf_validate(image, size);
    if (err) return err;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)image;
    const Elf64_Phdr *ph = (const Elf64_Phdr *)(image + eh->e_phoff);

    // Pass 1: check everything before touching memory.
    int entry_ok = 0;
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        if (ph[i].p_filesz > ph[i].p_memsz) return ELF_E_SEGMENT;
        if (ph[i].p_offset > size || ph[i].p_filesz > size - ph[i].p_offset) return ELF_E_SEGMENT;
        if (ph[i].p_vaddr < mem_base) return ELF_E_RANGE;
        uint64_t off = ph[i].p_vaddr - mem_base;
        if (off > mem_size || ph[i].p_memsz > mem_size - off) return ELF_E_RANGE;
        if (eh->e_entry >= ph[i].p_vaddr && eh->e_entry - ph[i].p_vaddr < ph[i].p_memsz) entry_ok = 1;
    }
    if (!entry_ok) return ELF_E_ENTRY;

    // Pass 2: copy the file bytes and zero the bss tail.
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        uint8_t *dst = memory + (ph[i].p_vaddr - mem_base);
        for (uint64_t j = 0; j < ph[i].p_filesz; j++) dst[j] = image[ph[i].p_offset + j];
        for (uint64_t j = ph[i].p_filesz; j < ph[i].p_memsz; j++) dst[j] = 0;
    }
    *entry = eh->e_entry;
    return ELF_OK;
}
