//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof M / sizeof M[0]; i++) {
        char name[64];
        snprintf(name, sizeof name, "sample %zu (%zu bytes)", i + 1, M[i].n);
        const char *got = identify(M[i].p, M[i].n);
        CHECK(name, got && strcmp(got, M[i].want) == 0, "expected \"%s\", got \"%s\"", M[i].want, got ? got : "(null)");
    }
    return 0;
}
