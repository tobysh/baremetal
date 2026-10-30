int main(void) {
    CHECK("set_bit(0, 0) is 1", set_bit(0, 0) == 1u, "got %u", (unsigned)set_bit(0, 0));
    CHECK("set_bit(0, 3) is 8", set_bit(0, 3) == 8u, "got %u", (unsigned)set_bit(0, 3));
    CHECK("set_bit(0, 31) is 0x80000000", set_bit(0, 31) == 0x80000000u, "got 0x%08X", (unsigned)set_bit(0, 31));
    CHECK("set_bit on an already-set bit changes nothing", set_bit(0xFFu, 4) == 0xFFu, "got 0x%X", (unsigned)set_bit(0xFFu, 4));
    CHECK("set_bit keeps the other bits (0x0F, 8 -> 0x10F)", set_bit(0x0Fu, 8) == 0x10Fu, "got 0x%X", (unsigned)set_bit(0x0Fu, 8));

    CHECK("clear_bit(15, 0) is 14", clear_bit(15, 0) == 14u, "got %u", (unsigned)clear_bit(15, 0));
    CHECK("clear_bit(0xFFFFFFFF, 31) is 0x7FFFFFFF", clear_bit(0xFFFFFFFFu, 31) == 0x7FFFFFFFu, "got 0x%08X", (unsigned)clear_bit(0xFFFFFFFFu, 31));
    CHECK("clear_bit on an already-clear bit changes nothing", clear_bit(0xF0u, 2) == 0xF0u, "got 0x%X", (unsigned)clear_bit(0xF0u, 2));
    CHECK("clear_bit(0xFFFFFFFF, 16) is 0xFFFEFFFF", clear_bit(0xFFFFFFFFu, 16) == 0xFFFEFFFFu, "got 0x%08X", (unsigned)clear_bit(0xFFFFFFFFu, 16));

    CHECK("toggle_bit(5, 1) is 7", toggle_bit(5, 1) == 7u, "got %u", (unsigned)toggle_bit(5, 1));
    CHECK("toggle_bit(7, 1) is 5", toggle_bit(7, 1) == 5u, "got %u", (unsigned)toggle_bit(7, 1));
    CHECK("toggle_bit(0, 31) is 0x80000000", toggle_bit(0, 31) == 0x80000000u, "got 0x%08X", (unsigned)toggle_bit(0, 31));
    CHECK("toggling twice restores 0xCAFEBABE", toggle_bit(toggle_bit(0xCAFEBABEu, 13), 13) == 0xCAFEBABEu, "got 0x%08X", (unsigned)toggle_bit(toggle_bit(0xCAFEBABEu, 13), 13));

    CHECK("test_bit(8, 3) is 1", test_bit(8, 3) == 1, "got %d", test_bit(8, 3));
    CHECK("test_bit(8, 2) is 0", test_bit(8, 2) == 0, "got %d", test_bit(8, 2));
    CHECK("test_bit(0x80000000, 31) is exactly 1", test_bit(0x80000000u, 31) == 1, "got %d", test_bit(0x80000000u, 31));
    CHECK("test_bit(0x80000000, 30) is 0", test_bit(0x80000000u, 30) == 0, "got %d", test_bit(0x80000000u, 30));
    CHECK("test_bit(0, 0) is 0", test_bit(0, 0) == 0, "got %d", test_bit(0, 0));

    int ok = 1;
    uint32_t pattern = 0xA5A5A5A5u;
    for (int n = 0; n < 32; n++) {
        int expected = (int)((pattern >> n) & 1u);
        if (test_bit(pattern, n) != expected) ok = 0;
    }
    CHECK("test_bit reads every bit of 0xA5A5A5A5 correctly", ok, "a bit position was read wrong");

    ok = 1;
    for (int n = 0; n < 32; n++) {
        uint32_t m = 1u << n;
        if (set_bit(0, n) != m) ok = 0;
        if (clear_bit(0xFFFFFFFFu, n) != ~m) ok = 0;
        if (toggle_bit(pattern, n) != (pattern ^ m)) ok = 0;
    }
    CHECK("set, clear and toggle are right for all 32 positions", ok, "a bit position was wrong");
    return 0;
}
