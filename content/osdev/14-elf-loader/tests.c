static uint8_t mem[0x3000] __attribute__((aligned(4096)));
static uint8_t img[1024];
#define BASE ((uint64_t)(uintptr_t)mem)
#define DATA_VADDR (BASE + 0x1000)

static void p16(int off, uint16_t v) { memcpy(img + off, &v, 2); }
static void p32(int off, uint32_t v) { memcpy(img + off, &v, 4); }
static void p64(int off, uint64_t v) { memcpy(img + off, &v, 8); }

static void phdr(int index, uint32_t type, uint64_t offset, uint64_t vaddr, uint64_t filesz, uint64_t memsz) {
    int o = 64 + index * 56;
    p32(o, type); p32(o + 4, 5); p64(o + 8, offset); p64(o + 16, vaddr); p64(o + 24, vaddr);
    p64(o + 32, filesz); p64(o + 40, memsz); p64(o + 48, 0x1000);
}

// A program: rax = data[0]; rcx = data[1] (bss, must be zero); return rax + rcx.
// Segment 0 is its code (offset 0x100), segment 1 its data (offset 0x200: the value 1234, then bss).
static size_t build(void) {
    memset(img, 0, sizeof img);
    img[0] = 0x7F; img[1] = 'E'; img[2] = 'L'; img[3] = 'F'; img[4] = 2; img[5] = 1; img[6] = 1;
    p16(16, 2); p16(18, 62); p32(20, 1);
    p64(24, BASE);            // entry
    p64(32, 64);              // phoff
    p16(52, 64); p16(54, 56); p16(56, 2);

    uint8_t *c = img + 0x100; int n = 0;
    c[n++] = 0x48; c[n++] = 0xA1; uint64_t a0 = DATA_VADDR; memcpy(c + n, &a0, 8); n += 8;       // movabs rax, [data]
    c[n++] = 0x48; c[n++] = 0xB9; uint64_t a1 = DATA_VADDR + 8; memcpy(c + n, &a1, 8); n += 8;   // mov rcx, &bss
    c[n++] = 0x48; c[n++] = 0x8B; c[n++] = 0x09;                                                 // mov rcx, [rcx]
    c[n++] = 0x48; c[n++] = 0x01; c[n++] = 0xC8;                                                 // add rax, rcx
    c[n++] = 0xC3;                                                                               // ret
    uint64_t value = 1234; memcpy(img + 0x200, &value, 8);

    phdr(0, PT_LOAD, 0x100, BASE, (uint64_t)n, (uint64_t)n);
    phdr(1, PT_LOAD, 0x200, DATA_VADDR, 8, 0x20);
    return 0x300;
}

static int mem_untouched(void) {
    for (size_t i = 0; i < sizeof mem; i++) if (mem[i] != 0xAA) return 0;
    return 1;
}

void kmain(void) {
    size_t size = build();
    CHECK("a well-formed image validates", elf_validate(img, size) == ELF_OK, "returned %d", elf_validate(img, size));
    CHECK("a file shorter than the header is ELF_E_SMALL", elf_validate(img, 63) == ELF_E_SMALL && elf_validate(img, 0) == ELF_E_SMALL, "returned %d", elf_validate(img, 63));

    build(); img[1] = 'X';
    CHECK("a wrong magic number is ELF_E_MAGIC", elf_validate(img, size) == ELF_E_MAGIC, "returned %d", elf_validate(img, size));
    build(); img[4] = 1;
    CHECK("a 32-bit file is ELF_E_CLASS", elf_validate(img, size) == ELF_E_CLASS, "returned %d", elf_validate(img, size));
    build(); img[5] = 2;
    CHECK("a big-endian file is ELF_E_CLASS", elf_validate(img, size) == ELF_E_CLASS, "returned %d", elf_validate(img, size));
    build(); p16(16, 3);
    CHECK("a shared object (ET_DYN) is ELF_E_TYPE", elf_validate(img, size) == ELF_E_TYPE, "returned %d", elf_validate(img, size));
    build(); p16(18, 3);
    CHECK("an i386 machine type is ELF_E_MACHINE", elf_validate(img, size) == ELF_E_MACHINE, "returned %d", elf_validate(img, size));
    build(); p16(54, 32);
    CHECK("a wrong program header entry size is ELF_E_PHDR", elf_validate(img, size) == ELF_E_PHDR, "returned %d", elf_validate(img, size));
    build(); p64(32, 0x2F0);
    CHECK("a program header table running off the end of the file is ELF_E_PHDR", elf_validate(img, size) == ELF_E_PHDR, "returned %d", elf_validate(img, size));
    build(); p64(32, 0xFFFFFFFFFFFFFFF0ULL);
    CHECK("a program header offset that would overflow is ELF_E_PHDR", elf_validate(img, size) == ELF_E_PHDR, "returned %d", elf_validate(img, size));
    build(); p16(56, 0xFFFF);
    CHECK("an enormous program header count is ELF_E_PHDR", elf_validate(img, size) == ELF_E_PHDR, "returned %d", elf_validate(img, size));

    // ---- loading ----
    uint64_t entry = 0;
    memset(mem, 0xAA, sizeof mem);
    size = build();
    int r = elf_load(img, size, mem, sizeof mem, BASE, &entry);
    CHECK("elf_load accepts the image", r == ELF_OK, "returned %d", r);
    CHECK("it reports the entry point", entry == BASE, "entry is 0x%lx, expected 0x%lx", entry, BASE);
    CHECK("the code segment is copied to its address", mem[0] == 0x48 && mem[1] == 0xA1, "first bytes are %02x %02x", mem[0], mem[1]);
    uint64_t v1, v2;
    memcpy(&v1, mem + 0x1000, 8); memcpy(&v2, mem + 0x1008, 8);
    CHECK("the data segment's initial value is copied", v1 == 1234, "value is %lu", v1);
    CHECK("the bss tail (memsz beyond filesz) is zeroed", v2 == 0 && mem[0x1000 + 0x1F] == 0, "bss word is 0x%lx", v2);
    CHECK("memory outside the segments is untouched", mem[0x1020] == 0xAA && mem[0x2000] == 0xAA && mem[0x80] == 0xAA, "bytes outside the segments were changed");

    int (*program)(void) = (int (*)(void))(uintptr_t)entry;
    int result = r == ELF_OK ? program() : -1;
    CHECK("jumping to the entry point runs the loaded program: 1234 + bss (0)", result == 1234, "the program returned %d", result);

    // ---- rejecting bad files without touching memory ----
    size = build(); phdr(1, PT_LOAD, 0x200, DATA_VADDR, 64, 8);
    memset(mem, 0xAA, sizeof mem);
    CHECK("filesz larger than memsz is ELF_E_SEGMENT", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_SEGMENT && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(1, PT_LOAD, 0x2F8, DATA_VADDR, 0x20, 0x20);
    memset(mem, 0xAA, sizeof mem);
    CHECK("file data running past the end of the image is ELF_E_SEGMENT", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_SEGMENT && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(1, PT_LOAD, 0xFFFFFFFFFFFFFFF0ULL, DATA_VADDR, 0x20, 0x20);
    memset(mem, 0xAA, sizeof mem);
    CHECK("an offset + filesz that wraps around is ELF_E_SEGMENT", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_SEGMENT && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(1, PT_LOAD, 0x200, BASE - 0x1000, 8, 0x20);
    memset(mem, 0xAA, sizeof mem);
    CHECK("a segment below the memory window is ELF_E_RANGE", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_RANGE && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(1, PT_LOAD, 0x200, BASE + 0x2FF0, 8, 0x20);
    memset(mem, 0xAA, sizeof mem);
    CHECK("a segment running past the end of the window is ELF_E_RANGE", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_RANGE && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(1, PT_LOAD, 0x200, 0xFFFFFFFFFFFFFFF0ULL, 8, 0x40);
    memset(mem, 0xAA, sizeof mem);
    CHECK("a segment whose vaddr + memsz wraps around is ELF_E_RANGE", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_RANGE && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(0, PT_LOAD, 0x100, BASE, 16, 16); p64(24, BASE + 0x500);
    memset(mem, 0xAA, sizeof mem);
    CHECK("an entry point outside every segment is ELF_E_ENTRY", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_ENTRY && mem_untouched(), "wrong result, or memory was modified");

    size = build(); p16(56, 0);
    memset(mem, 0xAA, sizeof mem);
    CHECK("an image with no segments has no entry point: ELF_E_ENTRY", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_ENTRY, "wrong result");

    size = build(); phdr(1, PT_LOAD, 0x200, BASE + 0x2FF0, 8, 0x20);
    memset(mem, 0xAA, sizeof mem);
    elf_load(img, size, mem, sizeof mem, BASE, &entry);
    CHECK("a bad second segment stops the load before the first is copied", mem_untouched(), "the code segment was copied although the image was rejected");

    size = build(); p16(18, 3);
    memset(mem, 0xAA, sizeof mem);
    CHECK("elf_load returns validation errors too", elf_load(img, size, mem, sizeof mem, BASE, &entry) == ELF_E_MACHINE && mem_untouched(), "wrong result, or memory was modified");

    size = build(); phdr(1, 4 /* PT_NOTE */, 0x200, 0xDEADBEEF00000000ULL, 0x5000, 0x5000);
    memset(mem, 0xAA, sizeof mem);
    r = elf_load(img, size, mem, sizeof mem, BASE, &entry);
    CHECK("segments that are not PT_LOAD are ignored, however odd they look", r == ELF_OK && mem[0] == 0x48, "returned %d", r);
}
