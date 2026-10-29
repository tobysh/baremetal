//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof I / sizeof I[0]; i++) {
        char name[64];
        Carved out[16];
        memset(out, 0, sizeof out);
        snprintf(name, sizeof name, "image %zu (%zu bytes)", i + 1, I[i].n);
        size_t got = carve(I[i].p, I[i].n, out, 16);
        int ok = got == (size_t)I[i].count;
        for (int k = 0; ok && k < I[i].count; k++)
            ok = out[k].type && strcmp(out[k].type, I[i].want[k].type) == 0 && out[k].offset == I[i].want[k].offset && out[k].length == I[i].want[k].length;
        if (ok) { CHECK(name, 1, ""); continue; }
        char msg[256] = "";
        int pos = 0;
        for (size_t k = 0; k < got && k < 16 && pos < 200; k++)
            pos += snprintf(msg + pos, sizeof msg - pos, "[%s @%zu +%zu] ", out[k].type ? out[k].type : "?", out[k].offset, out[k].length);
        CHECK(name, 0, "expected %d file(s) but found %zu: %s", I[i].count, got, msg);
    }
    {
        Carved out[2];
        size_t got = carve(I[0].p, I[0].n, out, 1);
        CHECK("never writes past max", got <= 1, "returned %zu with max 1", got);
        got = carve(I[1].p, 0, out, 2);
        CHECK("an empty image has nothing to carve", got == 0, "returned %zu", got);
    }
    return 0;
}
