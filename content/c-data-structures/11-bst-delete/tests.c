static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

static Node *mk(int key, Node *l, Node *r) {
    Node *n = malloc(sizeof *n);
    n->key = key; n->left = l; n->right = r;
    return n;
}
static int inorder(const Node *n, int *out, int pos) {
    if (!n) return pos;
    pos = inorder(n->left, out, pos);
    out[pos++] = n->key;
    return inorder(n->right, out, pos);
}
static int same(const Node *root, const int *want, int n) {
    int buf[600];
    int got = inorder(root, buf, 0);
    if (got != n) return 0;
    for (int i = 0; i < n; i++) if (buf[i] != want[i]) return 0;
    return 1;
}
static Node *build(const int *k, int n) {
    Node *r = NULL;
    for (int i = 0; i < n; i++) r = bst_insert(r, k[i]);
    return r;
}

int main(void) {
    CHECK("bst_min of an empty tree is NULL", bst_min(NULL) == NULL, "expected NULL");
    CHECK("bst_size of an empty tree is 0", bst_size(NULL) == 0, "got %d", bst_size(NULL));
    CHECK("an empty tree is valid", bst_is_valid(NULL) == 1, "expected 1");
    CHECK("deleting from an empty tree gives NULL", bst_delete(NULL, 3) == NULL, "expected NULL");

    int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    Node *t = build(keys, 10);
    CHECK("bst_size counts all nodes", bst_size(t) == 10, "got %d", bst_size(t));
    CHECK("bst_min finds 20", bst_min(t) != NULL && bst_min(t)->key == 20, "wrong minimum");
    CHECK("a tree built by insertion is valid", bst_is_valid(t) == 1, "expected 1");

    t = bst_delete(t, 999);
    int all[] = {20, 30, 35, 40, 45, 50, 60, 65, 70, 80};
    CHECK("deleting an absent key changes nothing", same(t, all, 10), "tree changed");

    t = bst_delete(t, 20);
    int a1[] = {30, 35, 40, 45, 50, 60, 65, 70, 80};
    CHECK("delete a leaf (20)", same(t, a1, 9) && bst_is_valid(t) && t->left->left == NULL, "wrong result");

    t = bst_delete(t, 60);
    int a2[] = {30, 35, 40, 45, 50, 65, 70, 80};
    CHECK("delete a node with one child (60 has only 65)", same(t, a2, 8) && bst_is_valid(t) && t->right->left != NULL && t->right->left->key == 65, "wrong result");

    t = bst_delete(t, 30);
    int a3[] = {35, 40, 45, 50, 65, 70, 80};
    CHECK("delete a node with a left child only (30 after 20 left)", same(t, a3, 7) && bst_is_valid(t), "wrong result");

    t = bst_delete(t, 50);
    int a4[] = {35, 40, 45, 65, 70, 80};
    CHECK("delete the root, which has two children", same(t, a4, 6) && bst_is_valid(t) && t->key == 65, "root is %d", t ? t->key : -1);

    t = bst_delete(t, 40);
    int a5[] = {35, 45, 65, 70, 80};
    CHECK("delete another node with two children", same(t, a5, 5) && bst_is_valid(t) && bst_size(t) == 5, "wrong result");
    bst_free(t);

    Node *single = bst_insert(NULL, 4);
    single = bst_delete(single, 4);
    CHECK("deleting the only node gives an empty tree", single == NULL, "expected NULL");

    int chain[] = {1, 2, 3, 4, 5};
    Node *c = build(chain, 5);
    c = bst_delete(c, 1);
    int cw[] = {2, 3, 4, 5};
    CHECK("deleting the root of a right-leaning chain", same(c, cw, 4) && c->key == 2, "root is %d", c ? c->key : -1);
    bst_free(c);

    Node *good = mk(10, mk(5, NULL, mk(7, NULL, NULL)), mk(15, NULL, NULL));
    CHECK("a hand-built valid tree is accepted", bst_is_valid(good) == 1, "expected 1");
    bst_free(good);
    Node *bad1 = mk(10, mk(5, NULL, mk(11, NULL, NULL)), mk(15, NULL, NULL));
    CHECK("11 inside the left subtree of 10 is rejected", bst_is_valid(bad1) == 0, "only parent-child checks are not enough");
    bst_free(bad1);
    Node *bad2 = mk(10, mk(5, NULL, NULL), mk(15, mk(9, NULL, NULL), NULL));
    CHECK("9 inside the right subtree of 10 is rejected", bst_is_valid(bad2) == 0, "expected 0");
    bst_free(bad2);
    Node *bad3 = mk(10, mk(10, NULL, NULL), NULL);
    CHECK("a duplicate key is rejected", bst_is_valid(bad3) == 0, "expected 0");
    bst_free(bad3);
    Node *bad4 = mk(5, mk(7, NULL, NULL), NULL);
    CHECK("a left child that is bigger is rejected", bst_is_valid(bad4) == 0, "expected 0");
    bst_free(bad4);
    Node *ext = mk(0, mk(-2147483647 - 1, NULL, NULL), mk(2147483647, NULL, NULL));
    CHECK("INT_MIN and INT_MAX keys are accepted", bst_is_valid(ext) == 1, "expected 1");
    bst_free(ext);
    Node *ext2 = mk(-2147483647 - 1, NULL, mk(-2147483647 - 1, NULL, NULL));
    CHECK("a duplicate INT_MIN is rejected", bst_is_valid(ext2) == 0, "expected 0");
    bst_free(ext2);

    unsigned seed = 31415u;
    Node *r = NULL;
    int model[600], mlen = 0;
    int ok = 1, step = 0;
    for (; step < 600 && ok; step++) {
        int k = (int)(rng(&seed) % 120u) - 60;
        int idx = -1;
        for (int i = 0; i < mlen; i++) if (model[i] == k) idx = i;
        if (rng(&seed) % 5u < 2u) {
            r = bst_insert(r, k);
            if (idx < 0) {
                int p = mlen;
                while (p > 0 && model[p - 1] > k) { model[p] = model[p - 1]; p--; }
                model[p] = k; mlen++;
            }
        } else {
            r = bst_delete(r, k);
            if (idx >= 0) { memmove(model + idx, model + idx + 1, (size_t)(mlen - idx - 1) * sizeof(int)); mlen--; }
        }
        if (!same(r, model, mlen) || bst_size(r) != mlen || !bst_is_valid(r)) ok = 0;
        if (mlen > 0 && bst_min(r)->key != model[0]) ok = 0;
    }
    CHECK("600 random inserts and deletes match a sorted reference", ok, "diverged at step %d", step);
    bst_free(r);
    return 0;
}
