//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof R / sizeof R[0]; i++) {
        char name[64], out[256];
        memset(out, 'Z', sizeof out);
        snprintf(name, sizeof name, "report for %s", R[i].name);
        report(R[i].name, R[i].p, R[i].n, R[i].expect, out, sizeof out);
        int terminated = memchr(out, '\0', sizeof out) != NULL;
        CHECK(name, terminated && strcmp(out, R[i].want) == 0, "expected: %sgot: %s", R[i].want, terminated ? out : "(not terminated)\n");
    }
    return 0;
}
