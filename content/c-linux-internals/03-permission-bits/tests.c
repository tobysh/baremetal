// Reference: decide each requested bit separately, class chosen by a small table.
static int r_access(const inode *n, const cred *c, int want) {
    for (int bit = 1; bit <= 4; bit <<= 1) {
        if (!(want & bit)) continue;
        int ok;
        if (c->euid == 0) {
            ok = (bit != P_X) || n->is_dir || (n->mode & 0111) != 0;
        } else {
            int cls = 2;
            if (c->euid == n->uid) cls = 0;
            else {
                int member = c->egid == n->gid;
                for (int i = 0; i < c->ngroups; i++) member |= c->groups[i] == n->gid;
                if (member) cls = 1;
            }
            ok = (n->mode >> (6 - 3 * cls)) & bit ? 1 : 0;
        }
        if (!ok) return 0;
    }
    return 1;
}

typedef struct { unsigned mode; unsigned uid, euid, gid, egid, grp; int want, dir, expect; const char *why; } golden;

int main(void) {
    // Hand-written expectations (not produced by the reference above).
    golden g[] = {
        {0640, 5, 5, 7, 7, 0, P_R,        0, 1, "owner reads rw-"},
        {0640, 5, 5, 7, 7, 0, P_X,        0, 0, "owner cannot execute without x"},
        {0070, 5, 5, 5, 7, 0, P_R,        0, 0, "owner class wins even when group would allow"},
        {0070, 5, 6, 5, 7, 0, P_R,        0, 1, "effective uid 6 is not the owner, group 7 matches"},
        {0004, 9, 9, 3, 3, 0, P_R,        0, 1, "others may read"},
        {0004, 5, 5, 7, 7, 0, P_R,        0, 0, "owner is denied although others may read"},
        {0050, 9, 9, 3, 3, 7, P_R | P_X,  0, 1, "supplementary group 7 grants r-x"},
        {0050, 9, 9, 3, 3, 7, P_W,        0, 0, "group has no write"},
        {0000, 0, 0, 0, 0, 0, P_R | P_W,  0, 1, "root reads and writes mode 000"},
        {0600, 0, 0, 0, 0, 0, P_X,        0, 0, "root cannot execute a file with no x bit"},
        {0601, 0, 0, 0, 0, 0, P_X,        0, 1, "root executes if anyone could"},
        {0000, 0, 0, 0, 0, 0, P_X,        1, 1, "root may search any directory"},
        {0755, 5, 5, 7, 7, 0, P_R | P_W,  0, 1, "owner rwx"},
        {0755, 9, 9, 7, 7, 0, P_R | P_W,  0, 0, "stranger r-x cannot write"},
    };
    for (unsigned i = 0; i < sizeof g / sizeof g[0]; i++) {
        inode n = {1, 5, 7, g[i].mode, g[i].dir};
        cred c = {g[i].uid, g[i].euid, g[i].gid, g[i].egid, {g[i].grp}, g[i].grp ? 1 : 0};
        int got = may_access(&n, &c, g[i].want);
        CHECK(g[i].why, got == g[i].expect, "mode %04o want %d: returned %d, expected %d", g[i].mode, g[i].want, got, g[i].expect);
    }

    // Exhaustive: every permission mode, several credentials, every request.
    cred cs[] = {
        {5, 5, 7, 7, {0}, 0},          // owner
        {9, 9, 7, 7, {0}, 0},          // primary group
        {9, 9, 3, 3, {4, 7}, 2},       // supplementary group
        {9, 9, 3, 3, {4}, 1},          // stranger
        {9, 5, 3, 3, {0}, 0},          // real uid differs, effective is the owner
        {5, 9, 3, 7, {0}, 0},          // real uid is the owner, effective is not; effective group matches
        {0, 0, 0, 0, {0}, 0},          // root
        {5, 0, 3, 3, {0}, 0},          // effective root
    };
    int bad = 0; char first[160] = "";
    for (unsigned mode = 0; mode < 01000; mode++) for (int d = 0; d < 2; d++) for (unsigned k = 0; k < sizeof cs / sizeof cs[0]; k++) for (int want = 1; want < 8; want++) {
        inode n = {1, 5, 7, mode, d};
        int got = may_access(&n, &cs[k], want), exp = r_access(&n, &cs[k], want);
        if (got != exp) { if (!bad) snprintf(first, sizeof first, "mode %04o dir %d cred #%u want %d: returned %d, expected %d", mode, d, k, want, got, exp); bad++; }
    }
    CHECK("all 512 modes x 8 credentials x 7 requests x file/dir agree with the reference", bad == 0, "%d mismatches, first: %s", bad, first);
    inode n = {1, 5, 7, 0000, 0}; cred c = {9, 9, 3, 3, {0}, 0};
    CHECK("an empty request is always allowed", may_access(&n, &c, 0) == 1, "returned 0");
    return 0;
}
