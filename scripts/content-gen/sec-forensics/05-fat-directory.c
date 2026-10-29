//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof D / sizeof D[0]; i++) {
        char name[64];
        DirEntry got[32];
        memset(got, 0, sizeof got);
        snprintf(name, sizeof name, "directory %zu (%d entries)", i + 1, D[i].count);
        int n = read_dir(D[i].p, D[i].n, got, 32);
        int ok = n == D[i].count;
        int bad = -1;
        for (int k = 0; ok && k < D[i].count; k++) {
            ok = strcmp(got[k].name, D[i].want[k].name) == 0 && got[k].deleted == D[i].want[k].deleted && got[k].is_dir == D[i].want[k].is_dir &&
                 got[k].size == D[i].want[k].size && got[k].cluster == D[i].want[k].cluster;
            if (!ok) bad = k;
        }
        if (n != D[i].count) CHECK(name, 0, "expected %d entries, got %d", D[i].count, n);
        else if (!ok) CHECK(name, 0, "entry %d: expected \"%s\" del=%d dir=%d size=%u cluster=%u, got \"%s\" del=%d dir=%d size=%u cluster=%u", bad,
                            D[i].want[bad].name, D[i].want[bad].deleted, D[i].want[bad].is_dir, D[i].want[bad].size, D[i].want[bad].cluster,
                            got[bad].name, got[bad].deleted, got[bad].is_dir, got[bad].size, got[bad].cluster);
        else CHECK(name, 1, "");
    }
    {
        DirEntry two[2];
        int n = read_dir(D[0].p, D[0].n, two, 2);
        CHECK("respects the max argument", n == 2, "returned %d with max 2", n);
    }
    return 0;
}
