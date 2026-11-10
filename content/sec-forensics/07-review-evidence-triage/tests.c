static const uint8_t e0[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x59, 0x9f, 0x5a, 0x73, 0x1d, 0x64, 0x1b, 0xe4,
    0x91, 0x10, 0x11, 0xdf, 0x60, 0x7b, 0x24, 0x3d, 0x1d, 0x8d, 0xda, 0x96, 0x94, 0x74, 0x0d, 0xaa,
    0xa5, 0x2e, 0x7c, 0x05, 0x19, 0xe3, 0x3e, 0xe9, 0xc9, 0x91, 0xd8, 0x88, 0x10, 0xc0,
};
static const uint8_t e1[] = {
    0x50, 0x4b, 0x03, 0x04, 0x67, 0x09, 0xfd, 0xe8, 0x5e, 0xf6, 0x64, 0x3a, 0x79, 0x23, 0xba,
};
static const uint8_t e2[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x49, 0x80, 0xc6, 0x6f, 0x93, 0x31, 0x86, 0x1a,
    0x72, 0x5b, 0xa3,
};
static const uint8_t e3[] = {
    0x25, 0x50, 0x44, 0x46, 0x2d, 0x3e, 0xee, 0xeb,
};
static const uint8_t e4[] = {
    0x50, 0x4b, 0x03, 0x04, 0x24, 0x92, 0x63, 0x0a, 0xfc, 0x8b, 0xe5, 0x87, 0xab, 0xf0, 0x5e, 0x40,
    0x91,
};
static const uint8_t e5[] = {
    0xff, 0xd8, 0xff, 0x31, 0xe1, 0xf3, 0xfb, 0x85, 0x1b, 0xcc, 0xc8, 0x94, 0xd9, 0x47, 0x5c, 0x42,
    0xd1, 0x2b, 0xf9, 0x4d, 0x3d, 0x35,
};
static const uint8_t e6[] = {
    0x00, 0x01, 0xf7, 0x66, 0xff, 0x91, 0xc3, 0xf8, 0x40, 0x8a, 0x7b, 0xae, 0x1b, 0x54, 0x23, 0x82,
    0x07, 0x7b, 0x3b, 0x6e, 0xaa, 0x61, 0x99, 0xea, 0x14, 0x05, 0x68, 0x1e, 0xb8, 0x41, 0xeb, 0x1d,
    0x9a, 0xdf, 0xa1, 0x2b, 0x66, 0x22, 0x9a,
};
static const uint8_t e7[] = {
    0xff,
};
static const uint8_t e8[] = {
    0x00,
};
static const struct { const char *name; const uint8_t *p; size_t n; uint64_t expect; const char *want; } R[] = {
    {"item1.bin", e0, 46, 0xceac4946b9e70234ULL, "item1.bin: PNG, first bytes 89 50 4e 47 0d 0a 1a 0a, hash ceac4946b9e70234, OK\n"},
    {"item2.bin", e1, 15, 0x56b42e46a0733002ULL, "item2.bin: ZIP, first bytes 50 4b 03 04 67 09 fd e8, hash 56b42e46a0733002, OK\n"},
    {"item3.bin", e2, 19, 0xcddad783c802a652ULL, "item3.bin: PNG, first bytes 89 50 4e 47 0d 0a 1a 0a, hash cddad7834802a652, CHANGED\n"},
    {"item4.bin", e3, 8, 0x781922d2da80aaceULL, "item4.bin: PDF, first bytes 25 50 44 46 2d 3e ee eb, hash 781923d2da80aace, CHANGED\n"},
    {"item5.bin", e4, 17, 0xf7e735a152e436cbULL, "item5.bin: ZIP, first bytes 50 4b 03 04 24 92 63 0a, hash f7e735a152e436cb, OK\n"},
    {"item6.bin", e5, 22, 0x7abf990d3193b653ULL, "item6.bin: JPEG, first bytes ff d8 ff 31 e1 f3 fb 85, hash 7abf990d3193b653, OK\n"},
    {"item7.bin", e6, 39, 0x654934cd3343befdULL, "item7.bin: unknown, first bytes 00 01 f7 66 ff 91 c3 f8, hash 654934cd33c3befd, CHANGED\n"},
    {"tiny.bin", e7, 1, 0xaf64724c8602eb6eULL, "tiny.bin: unknown, first bytes ff, hash af64724c8602eb6e, OK\n"},
    {"empty.bin", e8, 0, 0xcbf29ce484222325ULL, "empty.bin: unknown, first bytes -, hash cbf29ce484222325, OK\n"},
};

static uint64_t ref_fnv(const uint8_t *p, size_t n) {
    uint64_t h = 14695981039346656037ULL;
    while (n--) { h ^= *p++; h *= 1099511628211ULL; }
    return h;
}

int main(void) {
    for (size_t i = 0; i < sizeof R / sizeof R[0]; i++) {
        char name[64], out[256];
        memset(out, 'Z', sizeof out);
        snprintf(name, sizeof name, "report for %s", R[i].name);
        report(R[i].name, R[i].p, R[i].n, R[i].expect, out, sizeof out);
        int terminated = memchr(out, '\0', sizeof out) != NULL;
        CHECK(name, terminated && strcmp(out, R[i].want) == 0, "expected: %sgot: %s", R[i].want, terminated ? out : "(not terminated)\n");
    }

    /* Extra cases: expected lines are built from an independent reference inside the test. */
    {
        static const struct { const char *label, *type; uint8_t b[12]; size_t n; } T[] = {
            {"png", "PNG", {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a, 1, 2, 3, 4}, 12},
            {"jpeg", "JPEG", {0xff, 0xd8, 0xff, 0xe0, 0, 0x10, 'J', 'F', 'I', 'F', 0, 1}, 12},
            {"pdf", "PDF", {'%', 'P', 'D', 'F', '-', '1', '.', '7', 0x0a, 0, 0, 0}, 9},
            {"zip", "ZIP", {'P', 'K', 3, 4, 0x14, 0, 0, 0}, 8},
            {"gif", "unknown", {'G', 'I', 'F', '8', '9', 'a', 1, 0}, 8},
            {"elf", "unknown", {0x7f, 'E', 'L', 'F', 2, 1, 1, 0}, 8},
            {"truncated png", "unknown", {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a}, 7},
            {"truncated zip", "unknown", {'P', 'K', 3}, 3},
        };
        char out[512], want[512];
        for (size_t i = 0; i < sizeof T / sizeof T[0]; i++) {
            char hex[40] = "";
            size_t k = 0;
            for (size_t j = 0; j < T[i].n && j < 8; j++) k += (size_t)snprintf(hex + k, sizeof hex - k, "%s%02x", j ? " " : "", T[i].b[j]);
            uint64_t h = ref_fnv(T[i].b, T[i].n);
            snprintf(want, sizeof want, "%s.bin: %s, first bytes %s, hash %016llx, OK\n", T[i].label, T[i].type, hex, (unsigned long long)h);
            char nm[64];
            snprintf(nm, sizeof nm, "%s evidence reports type %s", T[i].label, T[i].type);
            memset(out, 'Z', sizeof out);
            char name[40];
            snprintf(name, sizeof name, "%s.bin", T[i].label);
            report(name, T[i].b, T[i].n, h, out, sizeof out);
            CHECK(nm, strcmp(out, want) == 0, "expected: %sgot: %s", want, out);
        }

        uint8_t nine[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9}, eight[8] = {0xab, 0xcd, 0xef, 0, 1, 2, 3, 0xff};
        report("nine", nine, 9, ref_fnv(nine, 9), out, sizeof out);
        CHECK("nine bytes show only the first eight", strstr(out, "first bytes 01 02 03 04 05 06 07 08, hash") != NULL, "got: %s", out);
        report("eight", eight, 8, ref_fnv(eight, 8), out, sizeof out);
        CHECK("eight bytes have no trailing space and use lowercase hex", strstr(out, "first bytes ab cd ef 00 01 02 03 ff, hash") != NULL, "got: %s", out);

        report("flip", nine, 9, ref_fnv(nine, 9) ^ 0x8000000000000000ULL, out, sizeof out);
        snprintf(want, sizeof want, "flip: unknown, first bytes 01 02 03 04 05 06 07 08, hash %016llx, CHANGED\n", (unsigned long long)ref_fnv(nine, 9));
        CHECK("a changed digest reports the computed value and CHANGED", strcmp(out, want) == 0, "expected: %sgot: %s", want, out);

        report("empty", nine, 0, ref_fnv(nine, 0), out, sizeof out);
        CHECK("empty evidence uses a dash and the initial state", strcmp(out, "empty: unknown, first bytes -, hash cbf29ce484222325, OK\n") == 0, "got: %s", out);

        /* capacity sweep: truncation keeps a prefix, terminates, and never writes outside cap */
        report("sweep.bin", eight, 8, 0, want, sizeof want);
        size_t full = strlen(want);
        int sweep_ok = 1;
        size_t bad_cap = 0;
        for (size_t cap = 0; cap <= full + 2 && sweep_ok; cap++) {
            uint8_t raw[8 + 200 + 8];
            memset(raw, 0xA5, sizeof raw);
            report("sweep.bin", eight, 8, 0, (char *)raw + 8, cap);
            size_t stored = cap == 0 ? 0 : (cap - 1 < full ? cap - 1 : full);
            int guards = 1;
            for (int g = 0; g < 8; g++) if (raw[g] != 0xA5 || raw[8 + cap + g] != 0xA5) guards = 0;
            int body = cap == 0 ? raw[8] == 0xA5 : (memcmp(raw + 8, want, stored) == 0 && raw[8 + stored] == 0);
            if (!guards || !body) { sweep_ok = 0; bad_cap = cap; }
        }
        CHECK("capacity sweep keeps the prefix, terminator and guards", sweep_ok, "cap %zu: output must be a zero-terminated prefix of the full line and nothing may be written outside cap", bad_cap);
        report("x", eight, 8, 0, NULL, 0);
        CHECK("cap zero with a NULL buffer writes nothing", 1, "");
    }

    return 0;
}
