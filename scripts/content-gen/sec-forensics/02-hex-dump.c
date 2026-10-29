//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof H / sizeof H[0]; i++) {
        char name[64], out[4096];
        snprintf(name, sizeof name, "dump %zu (%zu bytes)", i + 1, H[i].n);
        memset(out, 'Z', sizeof out);
        size_t len = hexdump(H[i].p, H[i].n, out, sizeof out);
        int terminated = memchr(out, '\0', sizeof out) != NULL;
        CHECK(name, terminated && len == strlen(H[i].want) && strcmp(out, H[i].want) == 0,
              "expected:\n%s(length %zu)\ngot (returned %zu):\n%.600s", H[i].want, strlen(H[i].want), len, terminated ? out : "(not terminated)");
    }
    return 0;
}
