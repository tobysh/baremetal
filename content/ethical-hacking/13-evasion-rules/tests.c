static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
static void hexpack(char *out, size_t cap, const char *inner, int key) {
    size_t n = (size_t)snprintf(out, cap, "PACK xor=%02x ", key);
    for (size_t i = 0; inner[i] && n + 3 < cap; i++) n += (size_t)snprintf(out + n, cap - n, "%02x", ((unsigned char)inner[i]) ^ key);
}
static const char *CASES[] = { "NET beacon", "net BEACON", "Net Beacon", "NET BEACON", "net beacon" };
static const char *HOSTS[] = { "update.example.test", "cdn7.example.test", "x.lab.test", "198.51.100.5" };
static const char *NOISE[] = { "SET mode=quiet", "FILE write /tmp/a.txt", "ECHO \"hello\"", "# build 14", "NET connect cdn.example.test", "SET host=a.example.test" };

// mask bits: 1 case mix, 2 odd spacing, 4 concatenation, 8 pack, 16 nested pack, 32 trailing garbage on the line
static void beacon_line(char *out, size_t cap, int mask, int hi) {
    const char *host = HOSTS[hi];
    char word[32] = "BEACON", head[16] = "NET", sep[16] = " ";
    if (mask & 1) { snprintf(head, sizeof head, "%s", (rnd() % 2) ? "Net" : "nEt"); snprintf(word, sizeof word, "%s", CASES[rnd() % 5] + 4); }
    if (mask & 2) snprintf(sep, sizeof sep, "%s", (rnd() % 2) ? " \t " : "\t  ");
    if (mask & 4) snprintf(word, sizeof word, "\"bea\"+\"con\"");
    snprintf(out, cap, "%s%s%s %s", head, sep, word, host);
    if (mask & 32) strncat(out, "  ", cap - strlen(out) - 1);
}

int main(void) {
    int badH = 0, badL = 0; char fH[220] = "", fL[220] = "";
    for (int it = 0; it < 400; it++) {
        rs = 31 + it * 101;
        int mask = (int)(rnd() % 64);
        if (mask & 16) mask |= 8;
        char line[200], body[1200] = "";
        beacon_line(line, sizeof line, mask, (int)(rnd() % 4));
        int before = (int)(rnd() % 4), after = (int)(rnd() % 3);
        for (int i = 0; i < before; i++) { strcat(body, NOISE[rnd() % 6]); strcat(body, "\n"); }
        strcat(body, line); strcat(body, "\n");
        for (int i = 0; i < after; i++) { strcat(body, NOISE[rnd() % 6]); strcat(body, "\n"); }
        char sample[4000];
        if (mask & 8) {
            char p1[2000]; hexpack(p1, sizeof p1, body, 0x20 + (int)(rnd() % 90));
            if (mask & 16) { char p2[3900]; hexpack(p2, sizeof p2, p1, 0x11 + (int)(rnd() % 80)); snprintf(sample, sizeof sample, "SET x=1\n%s\n", p2); }
            else snprintf(sample, sizeof sample, "# packed\n%s\n", p1);
        } else snprintf(sample, sizeof sample, "%s", body);
        if (!detect_v2(sample)) { if (!badH) snprintf(fH, sizeof fH, "mask %d missed: %.150s", mask, sample); badH++; }
        // The literal rule should only find the unobfuscated spelling.
        int plain = !(mask & (1 | 2 | 4 | 8 | 16)) && strstr(sample, "NET BEACON");
        if (detect_v1(sample) != (strstr(sample, "NET BEACON") != NULL) || (plain && !detect_v1(sample))) { if (!badL) snprintf(fL, sizeof fL, "mask %d", mask); badL++; }
    }
    CHECK("v1 is the literal NET BEACON rule", badL == 0, "%d samples handled differently from strstr, e.g. %s", badL, fL);
    CHECK("v2 catches disguised and packed variants", badH == 0, "%d of 400 variants slipped past, e.g. %s", badH, fH);

    {
        const char *plain = "SET host=a.example.test\nNET BEACON a.example.test\n";
        const char *v1 = "net  Beacon a.example.test\n", *v2 = "NET \tbeacon a.example.test", *v3 = "NET \"bea\"+\"con\" a.example.test\n";
        CHECK("the plain sample is caught by both rules", detect_v1(plain) && detect_v2(plain), "v1 %d, v2 %d", detect_v1(plain), detect_v2(plain));
        CHECK("the variants slip past v1", !detect_v1(v1) && !detect_v1(v2) && !detect_v1(v3), "v1 should be the plain literal rule, and so miss all three disguises");
        CHECK("but v2 still sees them", detect_v2(v1) && detect_v2(v2) && detect_v2(v3), "v2: %d %d %d", detect_v2(v1), detect_v2(v2), detect_v2(v3));
    }
    {
        const char *ok[] = {
            "# NET BEACON is disabled in this build\n", "ECHO \"NET BEACON\"\n", "NET connect update.example.test\n", "NET beaconing-docs x\n", "SET mode=net beacon\n",
            "", "\n\n", "NETBEACON x\n", "NET\nBEACON x\n", "   # net beacon\n", "FILE write /tmp/net beacon.txt\n", "net beacons x\n" };
        int bad = 0; char first[150] = "";
        for (size_t i = 0; i < sizeof ok / sizeof ok[0]; i++) if (detect_v2(ok[i])) { if (!bad) snprintf(first, sizeof first, "[%.60s]", ok[i]); bad++; }
        CHECK("v2 does not flag text that only mentions the words", bad == 0, "%d false alarms, e.g. %s", bad, first);
        CHECK("v1 does flag the mentions (the weakness you are fixing)", detect_v1(ok[0]) && detect_v1(ok[1]) && !detect_v1(ok[2]), "v1 is a plain substring search");
    }
    {
        char inner[200], p[500], s[600];
        snprintf(inner, sizeof inner, "ECHO hi\nNET beacon a.example.test\n");
        hexpack(p, sizeof p, inner, 0x5a);
        for (char *q = p + 12; *q; q++) *q = (char)toupper((unsigned char)*q);
        CHECK("upper-case hex digits are accepted", detect_v2(p), "hex digits may be upper case");
        snprintf(s, sizeof s, "pack XOR=5A %s", p + 12);
        CHECK("PACK keyword is case-insensitive", detect_v2(s), "pack xor=KK works in any case");
        char benign[100]; hexpack(benign, sizeof benign, "ECHO net beacon\n", 0x33);
        CHECK("a packed harmless script stays harmless", !detect_v2(benign), "decoded ECHO line is not a statement");
        char l1[300], l2[600], l3[1200], l4[2400], deep[2600];
        hexpack(l1, sizeof l1, "NET beacon x\n", 0x07); hexpack(l2, sizeof l2, l1, 0x08); hexpack(l3, sizeof l3, l2, 0x09); hexpack(l4, sizeof l4, l3, 0x0a);
        snprintf(deep, sizeof deep, "%s\n", l4);
        char three[1300]; snprintf(three, sizeof three, "%s\n", l3);
        CHECK("nesting is followed to three levels and no further", detect_v2(three) && !detect_v2(deep), "3 PACK layers are decoded; 4 are not (got %d and %d)", detect_v2(three), detect_v2(deep));
        CHECK("a bad hex pair stops decoding safely", !detect_v2("PACK xor=5a 1f3fzz4f\n") && !detect_v2("PACK xor=zz 1f3f\n") && !detect_v2("PACK xor=5"), "malformed packs are ignored without crashing");
    }
    return 0;
}
