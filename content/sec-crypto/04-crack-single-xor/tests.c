static const char *TEXTS[] = {
    "Cooking is really just chemistry that you are allowed to eat afterwards.",
    "The quick brown fox jumps over the lazy dog while the farmer watches from the hill.",
    "Never trust a cipher that its own designer cannot explain, and never design one yourself.",
    "In the morning the fog lay thick over the harbour, and the boats were hard to see.",
    "It was a bright cold day in April, and the clocks were striking thirteen.",
};
static const int KEYS[] = {0x58, 0x01, 0x7f, 0xc3, 0x20};

int main(void) {
    for (int t = 0; t < 5; t++) {
        uint8_t c[200];
        size_t n = strlen(TEXTS[t]);
        for (size_t i = 0; i < n; i++) c[i] = (uint8_t)(TEXTS[t][i] ^ KEYS[t]);
        int k = crack_xor(c, n);
        CHECK(t == 0 ? "finds key 0x58 for the cooking sentence" :
              t == 1 ? "finds key 0x01 (it only flips the lowest bit)" :
              t == 2 ? "finds key 0x7f" :
              t == 3 ? "finds key 0xc3 (every byte has its top bit set)" : "finds key 0x20 (it swaps letter case)",
              k == KEYS[t], "expected 0x%02x, got 0x%02x", KEYS[t], k);
    }
    for (int k = 0; k < 256; k += 51) {
        uint8_t c[200];
        const char *s = "Meet me by the old stone bridge at half past nine, and bring the map.";
        size_t n = strlen(s);
        for (size_t i = 0; i < n; i++) c[i] = (uint8_t)(s[i] ^ k);
        int g = crack_xor(c, n);
        if (g != k) { CHECK("finds every tested key for another sentence", 0, "key 0x%02x was reported as 0x%02x", k, g); return 0; }
    }
    CHECK("finds every tested key for another sentence", 1, "");
    return 0;
}
