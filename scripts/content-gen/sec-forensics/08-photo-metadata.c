//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof X / sizeof X[0]; i++) {
        char name[64];
        Meta m;
        memset(&m, 0x7e, sizeof m);
        snprintf(name, sizeof name, "photo %zu (%zu bytes)", i + 1, X[i].n);
        int rc = read_meta(X[i].p, X[i].n, &m);
        int ok = rc == X[i].rc;
        if (ok && rc == 0)
            ok = strcmp(m.make, X[i].want.make) == 0 && strcmp(m.model, X[i].want.model) == 0 && m.width == X[i].want.width &&
                 m.height == X[i].want.height && strcmp(m.datetime, X[i].want.datetime) == 0;
        CHECK(name, ok, "expected rc %d, make \"%s\", model \"%s\", %ux%u, taken \"%s\"; got rc %d, make \"%s\", model \"%s\", %ux%u, taken \"%s\"",
              X[i].rc, X[i].want.make, X[i].want.model, X[i].want.width, X[i].want.height, X[i].want.datetime,
              rc, rc == 0 ? m.make : "", rc == 0 ? m.model : "", rc == 0 ? m.width : 0, rc == 0 ? m.height : 0, rc == 0 ? m.datetime : "");
    }
    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++) {
        char name[64];
        snprintf(name, sizeof name, "to_unix(\"%.10s\")", T[i].s);
        long long got = to_unix(T[i].s);
        CHECK(name, got == T[i].want, "expected %lld, got %lld", T[i].want, got);
    }
    return 0;
}
