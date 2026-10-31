static int r_ma(const inode *n, const cred *c, int want) {
    for (int bit = 1; bit <= 4; bit <<= 1) {
        if (!(want & bit)) continue;
        int ok;
        if (c->euid == 0) ok = bit != P_X || n->is_dir || (n->mode & 0111);
        else {
            int cls = c->euid == n->uid ? 0 : (c->egid == n->gid ? 1 : 2);
            ok = (n->mode >> (6 - 3 * cls)) & bit ? 1 : 0;
        }
        if (!ok) return 0;
    }
    return 1;
}

static int r_unlink(const inode *d, const inode *f, const cred *c) {
    if (!r_ma(d, c, 3)) return 0;
    if (!(d->mode & 01000)) return 1;
    if (c->euid == 0) return 1;
    return c->euid == f->uid || c->euid == d->uid;
}

int main(void) {
    inode tmp = {1, 0, 0, 01777, 1}, af = {2, 1000, 100, 0000, 0};
    cred al = {1000, 1000, 100, 100, {0}, 0}, bob = {2000, 2000, 200, 200, {0}, 0}, root = {0, 0, 0, 0, {0}, 0};
    CHECK("an owner may remove their file from a sticky directory even if the file is mode 000", may_unlink(&tmp, &af, &al) == 1, "refused");
    CHECK("another user may not remove it from a sticky directory", may_unlink(&tmp, &af, &bob) == 0, "allowed");
    CHECK("root may", may_unlink(&tmp, &af, &root) == 1, "refused");
    inode plain = {1, 0, 0, 0777, 1};
    CHECK("without the sticky bit a writable directory lets anyone remove files", may_unlink(&plain, &af, &bob) == 1, "refused");
    inode ro = {1, 1000, 100, 0555, 1};
    CHECK("the owner of the file cannot remove it from a directory without write permission", may_unlink(&ro, &af, &al) == 0, "allowed");
    inode nox = {1, 1000, 100, 0600, 1};
    CHECK("write without search (x) on the directory is not enough", may_unlink(&nox, &af, &al) == 0, "allowed");
    inode dirowner = {1, 2000, 200, 01777, 1};
    CHECK("the directory's owner may remove anyone's file in a sticky directory", may_unlink(&dirowner, &af, &bob) == 1, "refused");
    inode notdir = {1, 1000, 100, 0777, 0};
    CHECK("the first argument must be a directory", may_unlink(&notdir, &af, &al) == 0, "allowed");

    // Sweep: directory modes x file owners x credentials against the reference.
    cred cs[] = {{1000, 1000, 100, 100, {0}, 0}, {2000, 2000, 200, 200, {0}, 0}, {2000, 1000, 200, 200, {0}, 0}, {0, 0, 0, 0, {0}, 0}, {7, 7, 100, 100, {0}, 0}};
    unsigned owners[] = {1000, 2000, 7, 0};
    int bad = 0; char first[160] = "";
    for (unsigned mode = 0; mode < 010000; mode++) for (unsigned k = 0; k < 5; k++) for (unsigned o = 0; o < 4; o++) {
        if ((mode & 07000) & ~01000) continue;
        inode d = {1, 1000, 100, mode, 1}, f = {2, owners[o], 100, 0644, 0};
        int g = may_unlink(&d, &f, &cs[k]), e = r_unlink(&d, &f, &cs[k]);
        if (g != e) { if (!bad) snprintf(first, sizeof first, "dir mode %04o cred #%u file owner %u: returned %d, expected %d", mode, k, owners[o], g, e); bad++; }
    }
    CHECK("sticky-directory unlink sweep agrees with the reference", bad == 0, "%d mismatches, first: %s", bad, first);

    // Exec.
    cred c = {2000, 2000, 200, 200, {5}, 1};
    inode su = {3, 0, 10, 04755, 0};
    int r = do_exec(&su, &c);
    CHECK("setuid changes only the effective uid", r == 0 && c.uid == 2000 && c.euid == 0 && c.egid == 200 && c.gid == 200, "ret %d uid %u euid %u egid %u", r, c.uid, c.euid, c.egid);
    c = (cred){2000, 2000, 200, 200, {5}, 1};
    inode sg = {3, 0, 10, 02755, 0};
    r = do_exec(&sg, &c);
    CHECK("setgid changes only the effective gid", r == 0 && c.euid == 2000 && c.egid == 10 && c.gid == 200 && c.groups[0] == 5, "ret %d euid %u egid %u", r, c.euid, c.egid);
    c = (cred){2000, 2000, 200, 200, {5}, 1};
    inode both = {3, 33, 44, 06711, 0};
    r = do_exec(&both, &c);
    CHECK("both bits apply together", r == 0 && c.euid == 33 && c.egid == 44, "euid %u egid %u", c.euid, c.egid);
    c = (cred){2000, 2000, 200, 200, {5}, 1};
    inode noexec = {3, 0, 10, 04700, 0};
    r = do_exec(&noexec, &c);
    CHECK("a setuid file the caller cannot execute is refused and changes nothing", r == -1 && c.euid == 2000 && c.egid == 200, "ret %d euid %u", r, c.euid);
    inode plainx = {3, 0, 10, 0755, 0};
    r = do_exec(&plainx, &c);
    CHECK("an ordinary program leaves credentials alone", r == 0 && c.euid == 2000 && c.egid == 200, "ret %d euid %u", r, c.euid);
    inode d = {3, 0, 10, 04755, 1};
    CHECK("a directory cannot be executed", do_exec(&d, &c) == -1 && c.euid == 2000, "accepted");
    c = (cred){2000, 2000, 200, 200, {0}, 0};
    inode ownerexec = {3, 2000, 200, 04100, 0};
    CHECK("the permission check uses the credentials from before the exec", do_exec(&ownerexec, &c) == 0 && c.euid == 2000, "unexpected result");
    int ok = 1;
    for (unsigned mode = 0; mode < 010000; mode++) {
        inode p = {3, 33, 44, mode, 0}; cred x = {2000, 2000, 200, 200, {0}, 0};
        int e = r_ma(&p, &x, P_X), got = do_exec(&p, &x);
        unsigned eu = (e && (mode & 04000)) ? 33 : 2000, eg = (e && (mode & 02000)) ? 44 : 200;
        if ((got == 0) != (e != 0) || x.euid != eu || x.egid != eg || x.uid != 2000) { ok = 0; break; }
    }
    CHECK("all 4096 modes behave as the reference says", ok, "a mode value misbehaves");
    return 0;
}
