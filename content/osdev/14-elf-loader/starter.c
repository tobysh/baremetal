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
    return ELF_E_SMALL;
}

int elf_load(const uint8_t *image, size_t size, uint8_t *memory, size_t mem_size, uint64_t mem_base, uint64_t *entry) {
    return ELF_E_SMALL;
}
