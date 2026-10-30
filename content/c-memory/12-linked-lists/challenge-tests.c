static Node *build(const int *v, int n, Node **nodes) {
    Node *head = NULL, *tail = NULL;
    for (int i = 0; i < n; i++) {
        Node *x = malloc(sizeof *x);
        x->value = v[i];
        x->next = NULL;
        nodes[i] = x;
        if (tail) tail->next = x; else head = x;
        tail = x;
    }
    return head;
}

static int values_are(const Node *h, const int *want, int n) {
    for (int i = 0; i < n; i++) {
        if (h == NULL || h->value != want[i]) return 0;
        h = h->next;
    }
    return h == NULL;
}

/* Every node in the result must be one of the original nodes, and none may be missing. */
static int relinked(const Node *h, Node **a, int na, Node **b, int nb) {
    int count = 0;
    for (; h != NULL; h = h->next, count++) {
        int found = 0;
        for (int i = 0; i < na; i++) if (a[i] == h) found = 1;
        for (int i = 0; i < nb; i++) if (b[i] == h) found = 1;
        if (!found || count > na + nb) return 0;
    }
    return count == na + nb;
}

int main(void) {
    Node *an[8], *bn[8];

    int a1[] = {1, 4, 9}, b1[] = {2, 3, 10}, w1[] = {1, 2, 3, 4, 9, 10};
    Node *r = merge_sorted(build(a1, 3, an), build(b1, 3, bn));
    CHECK("interleaved lists merge in order", values_are(r, w1, 6), "merged list is not 1 2 3 4 9 10");
    CHECK("the result reuses the original nodes (no malloc)", relinked(r, an, 3, bn, 3), "result contains a node that was not in either input, or lost one");

    int a2[] = {1, 2, 3}, b2[] = {7, 8}, w2[] = {1, 2, 3, 7, 8};
    r = merge_sorted(build(a2, 3, an), build(b2, 2, bn));
    CHECK("all of a before all of b", values_are(r, w2, 5), "merged list is not 1 2 3 7 8");

    r = merge_sorted(build(b2, 2, bn), build(a2, 3, an));
    CHECK("all of b before all of a (arguments swapped)", values_are(r, w2, 5), "merged list is not 1 2 3 7 8");

    int w3[] = {5, 6};
    r = merge_sorted(NULL, build(w3, 2, bn));
    CHECK("empty a returns b", values_are(r, w3, 2), "expected 5 6");
    r = merge_sorted(build(w3, 2, an), NULL);
    CHECK("empty b returns a", values_are(r, w3, 2), "expected 5 6");
    CHECK("both empty returns NULL", merge_sorted(NULL, NULL) == NULL, "expected NULL");

    int a4[] = {2, 2}, b4[] = {2, 2}, w4[] = {2, 2, 2, 2};
    r = merge_sorted(build(a4, 2, an), build(b4, 2, bn));
    CHECK("equal values keep a's nodes first", values_are(r, w4, 4) && r == an[0] && r->next == an[1], "ties must take the node from a first");

    int a5[] = {-5, -1, 0, 100}, b5[] = {-3, 0, 50}, w5[] = {-5, -3, -1, 0, 0, 50, 100};
    r = merge_sorted(build(a5, 4, an), build(b5, 3, bn));
    CHECK("negatives, zeros and duplicates", values_are(r, w5, 7) && relinked(r, an, 4, bn, 3), "merged list is not -5 -3 -1 0 0 50 100");
    return 0;
}
