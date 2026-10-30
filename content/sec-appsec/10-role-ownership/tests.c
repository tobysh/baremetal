static uint32_t rs = 31337;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }

// Independent reference, written as a table of rules rather than a switch.
static int want_allow(const User *u, int act, const Doc *d) {
    if (!u || u->role < 0 || u->role > 3 || act < 0 || act > 4) return 0;
    if (act == ACT_CREATE) return !d && u->role >= 1;
    if (!d) return 0;
    if (act == ACT_READ && d->is_public) return 1;
    if (u->org != d->org) return 0;
    if (d->locked && act != ACT_READ) return 0;
    if (u->role == 0) return 0;
    if (act == ACT_READ) return 1;
    int owner = u->uid == d->owner;
    if (act == ACT_EDIT) return owner || u->role >= 2;
    return owner || u->role == 3;
}

int main(void) {
    // Exhaustive over a small world.
    {
        int bad = 0, tot = 0; char first[160] = "";
        for (int role = 0; role < 4; role++) for (int uorg = 1; uorg <= 2; uorg++) for (int uid = 1; uid <= 3; uid++)
        for (int act = 0; act < 5; act++) for (int owner = 1; owner <= 3; owner++) for (int dorg = 1; dorg <= 2; dorg++)
        for (int pub = 0; pub < 2; pub++) for (int lock = 0; lock < 2; lock++) {
            User u = { uid, uorg, role }; Doc d = { 10 + owner, owner, dorg, pub, lock };
            int want = want_allow(&u, act, act == ACT_CREATE ? NULL : &d);
            int got = authorize(&u, act, act == ACT_CREATE ? NULL : &d) != 0;
            tot++;
            if (got != want) { if (!bad) snprintf(first, sizeof first, "role %d uid %d org %d, action %d, doc owner %d org %d public %d locked %d: expected %d", role, uid, uorg, act, owner, dorg, pub, lock, want); bad++; }
        }
        CHECK("every role, action and document combination", bad == 0, "%d of %d decisions wrong, e.g. %s", bad, tot, first);
    }
    // Deny by default.
    {
        User m = { 1, 1, ROLE_ADMIN }; Doc d = { 1, 1, 1, 1, 0 };
        CHECK("no user means no access", !authorize(NULL, ACT_READ, &d) && !authorize(NULL, ACT_CREATE, NULL), "a missing user must be denied, even for public documents");
        int bad = 0;
        int junk_roles[] = { -1, 4, 99, 1000, -1000 };
        for (int i = 0; i < 5; i++) { User j = { 1, 1, junk_roles[i] }; if (authorize(&j, ACT_READ, &d) || authorize(&j, ACT_CREATE, NULL)) bad++; }
        CHECK("an unknown role is denied", bad == 0, "%d junk roles got access; unknown values must not fall through to allow", bad);
        int acts[] = { -1, 5, 42, 1 << 20 }; bad = 0;
        for (int i = 0; i < 4; i++) if (authorize(&m, acts[i], &d)) bad++;
        CHECK("an unknown action is denied", bad == 0, "even an admin cannot perform an action that does not exist");
        CHECK("a missing document is denied", !authorize(&m, ACT_READ, NULL) && !authorize(&m, ACT_EDIT, NULL) && !authorize(&m, ACT_DELETE, NULL), "only CREATE takes no document");
        CHECK("create must not name a document", !authorize(&m, ACT_CREATE, &d), "CREATE is for new objects: a document pointer is a misuse");
    }
    // Admins stop at the organisation boundary.
    {
        User adm = { 1, 1, ROLE_ADMIN }; Doc other = { 5, 9, 2, 0, 0 };
        CHECK("an admin cannot reach into another organisation", !authorize(&adm, ACT_READ, &other) && !authorize(&adm, ACT_EDIT, &other) && !authorize(&adm, ACT_DELETE, &other) && !authorize(&adm, ACT_SHARE, &other), "roles apply inside an organisation only");
        other.is_public = 1;
        CHECK("public documents may be read by anyone, but not changed", authorize(&adm, ACT_READ, &other) && !authorize(&adm, ACT_EDIT, &other), "public means readable");
        User g = { 3, 2, ROLE_GUEST };
        CHECK("a guest may read public documents only", authorize(&g, ACT_READ, &other) && !authorize(&g, ACT_CREATE, NULL), "guests read; they do not create");
        Doc own = { 6, 3, 2, 0, 0 };
        CHECK("owning a document does not help a guest", !authorize(&g, ACT_READ, &own) && !authorize(&g, ACT_EDIT, &own), "guest is denied inside the organisation");
    }
    // List filtering uses the same check.
    {
        int bad = 0, tot = 0; char first[160] = "";
        for (int t = 0; t < 60; t++) {
            Doc docs[30]; size_t n = rnd() % 31;
            for (size_t i = 0; i < n; i++) { docs[i].id = (int)i; docs[i].owner = 1 + (int)(rnd() % 4); docs[i].org = 1 + (int)(rnd() % 3); docs[i].is_public = rnd() % 5 == 0; docs[i].locked = rnd() % 4 == 0; }
            User u = { 1 + (int)(rnd() % 4), 1 + (int)(rnd() % 3), (int)(rnd() % 4) };
            size_t idx[32]; size_t guard[2] = { 0xAAAA, 0xBBBB };
            size_t got = filter_readable(&u, docs, n, idx), want = 0, wi[32]; (void)guard;
            for (size_t i = 0; i < n; i++) if (want_allow(&u, ACT_READ, &docs[i])) wi[want++] = i;
            tot++;
            int ok = got == want; for (size_t i = 0; ok && i < want; i++) if (idx[i] != wi[i]) ok = 0;
            if (!ok) { if (!bad) snprintf(first, sizeof first, "list %d: expected %zu documents, got %zu", t, want, got); bad++; }
        }
        CHECK("lists only contain documents the caller may read", bad == 0, "%d of %d lists wrong, e.g. %s", bad, tot, first);
        User ua = { 1, 1, ROLE_ADMIN };
        CHECK("an empty list works", filter_readable(&ua, NULL, 0, NULL) == 0, "n = 0 must return 0");
    }
    return 0;
}
