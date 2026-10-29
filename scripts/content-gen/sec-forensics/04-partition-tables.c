//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof B / sizeof B[0]; i++) {
        char name[64];
        Partition got[4];
        memset(got, 0xEE, sizeof got);
        snprintf(name, sizeof name, "disk %zu", i + 1);
        int n = parse_mbr(B[i].p, got);
        int ok = n == B[i].count;
        for (int k = 0; ok && k < B[i].count; k++)
            ok = got[k].bootable == B[i].want[k].bootable && got[k].type == B[i].want[k].type &&
                 got[k].start_lba == B[i].want[k].start_lba && got[k].sectors == B[i].want[k].sectors;
        char msg[200] = "";
        int pos = 0;
        for (int k = 0; k < n && k < 4; k++)
            pos += snprintf(msg + pos, sizeof msg - pos, "[boot %d type 0x%02x lba %u n %u] ", got[k].bootable, got[k].type, got[k].start_lba, got[k].sectors);
        CHECK(name, ok, "expected %d partition(s) (-1 = not an MBR), got %d: %s", B[i].count, n, msg);
    }
    for (size_t i = 0; i < sizeof N / sizeof N[0]; i++) {
        char name[64];
        snprintf(name, sizeof name, "type 0x%02x is named", N[i].type);
        const char *got = type_name(N[i].type);
        CHECK(name, got && strcmp(got, N[i].name) == 0, "expected \"%s\", got \"%s\"", N[i].name, got ? got : "(null)");
    }
    return 0;
}
