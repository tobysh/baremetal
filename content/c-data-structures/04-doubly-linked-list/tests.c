static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

/* Check forward and backward walks against an expected array, plus all invariants. */
static int matches(const DList *l, const int *want, int n) {
    if (l->len != (size_t)n) return 0;
    if (n == 0) return l->head == NULL && l->tail == NULL;
    if (l->head == NULL || l->tail == NULL || l->head->prev != NULL || l->tail->next != NULL) return 0;
    const Node *p = l->head;
    for (int i = 0; i < n; i++) {
        if (p == NULL || p->val != want[i]) return 0;
        if (p->next && p->next->prev != p) return 0;
        if (i == n - 1 && p != l->tail) return 0;
        p = p->next;
    }
    if (p != NULL) return 0;
    p = l->tail;
    for (int i = n - 1; i >= 0; i--) {
        if (p == NULL || p->val != want[i]) return 0;
        p = p->prev;
    }
    return p == NULL;
}

int main(void) {
    DList l;
    dl_init(&l);
    CHECK("a new list is empty", matches(&l, NULL, 0), "head/tail/len are wrong");
    CHECK("dl_find on an empty list gives NULL", dl_find(&l, 1) == NULL, "expected NULL");

    CHECK("push_back into an empty list", dl_push_back(&l, 5) == 0 && l.head == l.tail && l.head != NULL, "head and tail should be the same node");
    int w1[] = {5};
    CHECK("a one-node list is consistent", matches(&l, w1, 1), "links are wrong");
    dl_push_back(&l, 6);
    dl_push_back(&l, 7);
    dl_push_front(&l, 4);
    int w2[] = {4, 5, 6, 7};
    CHECK("push_back and push_front build 4 5 6 7", matches(&l, w2, 4), "list is not 4 5 6 7 with consistent links");

    Node *six = dl_find(&l, 6);
    CHECK("dl_find locates a value", six != NULL && six->val == 6 && dl_find(&l, 99) == NULL, "wrong result");
    dl_insert_after(&l, six, 60);
    dl_insert_after(&l, l.tail, 8);
    dl_insert_after(&l, l.head, 45);
    int w3[] = {4, 45, 5, 6, 60, 7, 8};
    CHECK("insert_after in the middle, at the tail and after the head", matches(&l, w3, 7), "list is not 4 45 5 6 60 7 8 with consistent links");

    Node *keep = dl_find(&l, 5);
    dl_remove(&l, l.head);
    int w4[] = {45, 5, 6, 60, 7, 8};
    CHECK("removing the head", matches(&l, w4, 6), "bad list after removing the head");
    dl_remove(&l, l.tail);
    int w5[] = {45, 5, 6, 60, 7};
    CHECK("removing the tail", matches(&l, w5, 5), "bad list after removing the tail");
    dl_remove(&l, dl_find(&l, 60));
    int w6[] = {45, 5, 6, 7};
    CHECK("removing a middle node", matches(&l, w6, 4), "bad list after removing a middle node");
    CHECK("other nodes keep their addresses", dl_find(&l, 5) == keep, "node moved");

    dl_reverse(&l);
    int w7[] = {7, 6, 5, 45};
    CHECK("dl_reverse reverses both directions", matches(&l, w7, 4), "list is not 7 6 5 45 with consistent links");
    CHECK("dl_reverse reuses the same nodes", dl_find(&l, 5) == keep, "nodes were reallocated");
    dl_reverse(&l);
    CHECK("reversing twice restores the order", matches(&l, w6, 4), "not back to 45 5 6 7");

    DList e;
    dl_init(&e);
    dl_reverse(&e);
    CHECK("reversing an empty list is safe", matches(&e, NULL, 0), "list not empty");
    int w8[] = {1};
    dl_push_back(&e, 1);
    dl_reverse(&e);
    CHECK("reversing a single node is safe", matches(&e, w8, 1), "single node broken");
    dl_remove(&e, e.head);
    CHECK("removing the only node empties the list", matches(&e, NULL, 0), "head/tail/len wrong");
    dl_free(&e);

    dl_free(&l);
    CHECK("dl_free leaves an empty list", matches(&l, NULL, 0), "list not empty");

    DList r;
    dl_init(&r);
    int model[400], mlen = 0;
    unsigned seed = 4242u;
    int ok = 1, step = 0;
    for (; step < 400 && ok; step++) {
        unsigned op = rng(&seed) % 5u;
        int v = (int)(rng(&seed) % 1000u);
        if (op == 0) { dl_push_back(&r, v); model[mlen++] = v; }
        else if (op == 1) { dl_push_front(&r, v); memmove(model + 1, model, (size_t)mlen * sizeof(int)); model[0] = v; mlen++; }
        else if (op == 2 && mlen > 0) {
            int i = (int)(rng(&seed) % (unsigned)mlen);
            Node *p = r.head;
            for (int k = 0; k < i; k++) p = p->next;
            dl_insert_after(&r, p, v);
            memmove(model + i + 2, model + i + 1, (size_t)(mlen - i - 1) * sizeof(int));
            model[i + 1] = v; mlen++;
        } else if (op == 3 && mlen > 0) {
            int i = (int)(rng(&seed) % (unsigned)mlen);
            Node *p = r.head;
            for (int k = 0; k < i; k++) p = p->next;
            dl_remove(&r, p);
            memmove(model + i, model + i + 1, (size_t)(mlen - i - 1) * sizeof(int));
            mlen--;
        } else if (op == 4) {
            dl_reverse(&r);
            for (int a = 0, b = mlen - 1; a < b; a++, b--) { int t = model[a]; model[a] = model[b]; model[b] = t; }
        }
        if (!matches(&r, model, mlen)) ok = 0;
    }
    CHECK("400 random operations stay consistent with a reference array", ok, "diverged at step %d", step);
    dl_free(&r);
    return 0;
}
