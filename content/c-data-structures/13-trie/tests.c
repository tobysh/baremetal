static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    Trie *t = trie_new();
    CHECK("trie_new returns a node", t != NULL, "got NULL");
    if (t == NULL) return 0;
    CHECK("an empty trie contains nothing", !trie_contains(t, "a") && !trie_has_prefix(t, "a") && trie_count_prefix(t, "") == 0, "found something");

    CHECK("insert car, cat, do", trie_insert(t, "car") == 0 && trie_insert(t, "cat") == 0 && trie_insert(t, "do") == 0, "returned non-zero");
    CHECK("inserted words are contained", trie_contains(t, "car") && trie_contains(t, "cat") && trie_contains(t, "do"), "a word is missing");
    CHECK("a prefix of a word is not a word", !trie_contains(t, "ca") && !trie_contains(t, "c") && !trie_contains(t, "d"), "prefix reported as word");
    CHECK("an extension of a word is not a word", !trie_contains(t, "cars") && !trie_contains(t, "dog") && !trie_contains(t, "cab"), "extension reported as word");
    CHECK("has_prefix finds real prefixes", trie_has_prefix(t, "c") && trie_has_prefix(t, "ca") && trie_has_prefix(t, "car") && trie_has_prefix(t, "do"), "prefix missing");
    CHECK("has_prefix rejects absent prefixes", !trie_has_prefix(t, "cb") && !trie_has_prefix(t, "x") && !trie_has_prefix(t, "cart"), "false prefix accepted");
    CHECK("count_prefix: ca gives 2, c gives 2, d gives 1", trie_count_prefix(t, "ca") == 2 && trie_count_prefix(t, "c") == 2 && trie_count_prefix(t, "d") == 1, "got %d %d %d", trie_count_prefix(t, "ca"), trie_count_prefix(t, "c"), trie_count_prefix(t, "d"));
    CHECK("count_prefix of the empty prefix counts all words", trie_count_prefix(t, "") == 3, "got %d", trie_count_prefix(t, ""));
    CHECK("count_prefix of a full word counts it once", trie_count_prefix(t, "car") == 1, "got %d", trie_count_prefix(t, "car"));

    CHECK("inserting a duplicate is fine and not double counted", trie_insert(t, "cat") == 0 && trie_count_prefix(t, "") == 3, "got %d", trie_count_prefix(t, ""));
    CHECK("a word that is a prefix of another can be added", trie_insert(t, "ca") == 0 && trie_contains(t, "ca") && trie_contains(t, "car") && trie_count_prefix(t, "ca") == 3, "count %d", trie_count_prefix(t, "ca"));
    CHECK("a longer word through an existing path", trie_insert(t, "cartoon") == 0 && trie_contains(t, "cartoon") && trie_contains(t, "car") && trie_count_prefix(t, "car") == 2, "count %d", trie_count_prefix(t, "car"));

    CHECK("empty words are rejected", trie_insert(t, "") == -1 && !trie_contains(t, ""), "accepted the empty word");
    CHECK("uppercase is rejected", trie_insert(t, "Dog") == -1 && !trie_contains(t, "Dog"), "accepted uppercase");
    CHECK("a bad character after a good prefix is rejected", trie_insert(t, "zeb1ra") == -1, "accepted a digit");
    CHECK("a rejected word leaves no nodes behind", !trie_has_prefix(t, "z") && !trie_has_prefix(t, "zeb") && trie_count_prefix(t, "") == 5, "partial path remained: has z? %d", trie_has_prefix(t, "z"));
    CHECK("queries with invalid characters answer no", !trie_contains(t, "c-t") && !trie_has_prefix(t, "ca!") && trie_count_prefix(t, "C") == 0, "invalid query matched");
    CHECK("a space is invalid", trie_insert(t, "ice cream") == -1 && !trie_has_prefix(t, "ice"), "accepted a space");

    Trie *r = trie_new();
    char words[200][8];
    int nw = 0;
    unsigned seed = 5551212u;
    int ok = 1;
    for (int i = 0; i < 200; i++) {
        int len = 1 + (int)(rng(&seed) % 4u);
        for (int k = 0; k < len; k++) words[nw][k] = (char)('a' + rng(&seed) % 3u);
        words[nw][len] = '\0';
        if (trie_insert(r, words[nw]) != 0) ok = 0;
        nw++;
    }
    CHECK("200 short random words over 'abc' insert fine", ok, "an insert failed");
    char probe[8];
    int bad = 0, probes = 0;
    for (int len = 1; len <= 5 && !bad; len++) {
        int total = 1;
        for (int k = 0; k < len; k++) total *= 3;
        for (int code = 0; code < total && !bad; code++) {
            int c = code;
            for (int k = 0; k < len; k++) { probe[k] = (char)('a' + c % 3); c /= 3; }
            probe[len] = '\0';
            int exact = 0, pref = 0, seen_dup = 0;
            for (int i = 0; i < nw; i++) {
                int dup = 0;
                for (int j = 0; j < i; j++) if (strcmp(words[i], words[j]) == 0) dup = 1;
                if (dup) continue;
                if (strcmp(words[i], probe) == 0) exact = 1;
                if (strncmp(words[i], probe, (size_t)len) == 0) pref++;
            }
            (void)seen_dup;
            probes++;
            if (trie_contains(r, probe) != exact || trie_count_prefix(r, probe) != pref || trie_has_prefix(r, probe) != (pref > 0)) bad = probes;
        }
    }
    CHECK("every string over 'abc' up to length 5 agrees with a brute-force scan", !bad, "mismatch at probe %d (%s)", bad, probe);
    trie_free(r);
    trie_free(t);
    trie_free(NULL);
    return 0;
}
