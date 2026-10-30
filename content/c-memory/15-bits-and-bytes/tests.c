int main(void) {
    CHECK("count_bits(0) is 0", count_bits(0) == 0, "got %d", count_bits(0));
    CHECK("count_bits(1) is 1", count_bits(1) == 1, "got %d", count_bits(1));
    CHECK("count_bits(7) is 3", count_bits(7) == 3, "got %d", count_bits(7));
    CHECK("count_bits(255) is 8", count_bits(255) == 8, "got %d", count_bits(255));
    CHECK("count_bits(0x80000000) is 1 (top bit)", count_bits(0x80000000u) == 1, "got %d", count_bits(0x80000000u));
    CHECK("count_bits(0xAAAAAAAA) is 16", count_bits(0xAAAAAAAAu) == 16, "got %d", count_bits(0xAAAAAAAAu));
    CHECK("count_bits(0xFFFFFFFF) is 32", count_bits(0xFFFFFFFFu) == 32, "got %d", count_bits(0xFFFFFFFFu));

    CHECK("is_power_of_two(0) is 0", is_power_of_two(0) == 0, "got %d", is_power_of_two(0));
    CHECK("is_power_of_two(1) is 1", is_power_of_two(1) == 1, "got %d", is_power_of_two(1));
    CHECK("is_power_of_two(2) is 1", is_power_of_two(2) == 1, "got %d", is_power_of_two(2));
    CHECK("is_power_of_two(3) is 0", is_power_of_two(3) == 0, "got %d", is_power_of_two(3));
    CHECK("is_power_of_two(64) is 1", is_power_of_two(64) == 1, "got %d", is_power_of_two(64));
    CHECK("is_power_of_two(100) is 0", is_power_of_two(100) == 0, "got %d", is_power_of_two(100));
    CHECK("is_power_of_two(1024) is 1", is_power_of_two(1024) == 1, "got %d", is_power_of_two(1024));
    CHECK("is_power_of_two(0x80000000) is 1", is_power_of_two(0x80000000u) == 1, "got %d", is_power_of_two(0x80000000u));
    CHECK("is_power_of_two(0xFFFFFFFF) is 0", is_power_of_two(0xFFFFFFFFu) == 0, "got %d", is_power_of_two(0xFFFFFFFFu));

    CHECK("swap_endian(0x12345678) is 0x78563412", swap_endian(0x12345678u) == 0x78563412u, "got 0x%08X", (unsigned)swap_endian(0x12345678u));
    CHECK("swap_endian(0) is 0", swap_endian(0) == 0, "got 0x%08X", (unsigned)swap_endian(0));
    CHECK("swap_endian(0xFFFFFFFF) is 0xFFFFFFFF", swap_endian(0xFFFFFFFFu) == 0xFFFFFFFFu, "got 0x%08X", (unsigned)swap_endian(0xFFFFFFFFu));
    CHECK("swap_endian(0x000000FF) is 0xFF000000", swap_endian(0x000000FFu) == 0xFF000000u, "got 0x%08X", (unsigned)swap_endian(0x000000FFu));
    CHECK("swap_endian(0xDEADBEEF) is 0xEFBEADDE", swap_endian(0xDEADBEEFu) == 0xEFBEADDEu, "got 0x%08X", (unsigned)swap_endian(0xDEADBEEFu));
    CHECK("swap_endian twice restores 0x01020304", swap_endian(swap_endian(0x01020304u)) == 0x01020304u, "got 0x%08X", (unsigned)swap_endian(swap_endian(0x01020304u)));
    return 0;
}
