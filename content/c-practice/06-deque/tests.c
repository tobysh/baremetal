// Returns 1 if the deque holds exactly want[0..n), with every link correct in both directions.
static int is(const Deque *d, const int *want, int n) {
    if (d->size != n) return 0;
    DNode *prev = NULL;
    DNode *cur = d->head;
    for (int i = 0; i < n; i++) {
        if (cur == NULL || cur->value != want[i] || cur->prev != prev) return 0;
        prev = cur;
        cur = cur->next;
    }
    if (cur != NULL) return 0;
    if (d->tail != prev) return 0;
    if (n == 0 && (d->head != NULL || d->tail != NULL)) return 0;
    return 1;
}

int main(void) {
    Deque d;
    deque_init(&d);
    CHECK("deque_init makes an empty deque", d.head == NULL && d.tail == NULL && d.size == 0, "head/tail/size not cleared");
    int out = -1;
    CHECK("pop_front on an empty deque fails and leaves out alone", pop_front(&d, &out) == -1 && out == -1, "out = %d", out);
    CHECK("pop_back on an empty deque fails and leaves out alone", pop_back(&d, &out) == -1 && out == -1, "out = %d", out);

    int ok = push_back(&d, 2) == 0 && push_back(&d, 3) == 0 && push_front(&d, 1) == 0;
    int w1[] = {1, 2, 3};
    CHECK("push_front and push_back build 1 <-> 2 <-> 3 with correct links", ok && is(&d, w1, 3), "list is not 1, 2, 3 with consistent links (size = %d)", d.size);

    int w2[] = {0, 1, 2, 3, 4};
    push_front(&d, 0);
    push_back(&d, 4);
    CHECK("pushing at both ends of a longer list", is(&d, w2, 5), "list is not 0..4 with consistent links (size = %d)", d.size);

    CHECK("pop_front returns the first value", pop_front(&d, &out) == 0 && out == 0, "out = %d", out);
    CHECK("pop_back returns the last value", pop_back(&d, &out) == 0 && out == 4, "out = %d", out);
    CHECK("after popping both ends the list is 1, 2, 3", is(&d, w1, 3), "links or size wrong (size = %d)", d.size);

    Deque e;
    deque_init(&e);
    push_back(&e, 7);
    CHECK("pop_front of the only node empties the deque completely", pop_front(&e, &out) == 0 && out == 7 && e.head == NULL && e.tail == NULL && e.size == 0, "head/tail/size not reset");
    push_front(&e, 8);
    CHECK("pop_back of the only node empties the deque completely", pop_back(&e, &out) == 0 && out == 8 && e.head == NULL && e.tail == NULL && e.size == 0, "head/tail/size not reset");
    CHECK("a deque can be reused after emptying", push_back(&e, 9) == 0 && e.head == e.tail && e.head != NULL && e.size == 1, "reuse failed");
    deque_free(&e);

    Deque r;
    deque_init(&r);
    for (int i = 1; i <= 5; i++) push_back(&r, i * 10);
    CHECK("deque_remove of a missing value returns 0 and changes nothing", deque_remove(&r, 99) == 0 && r.size == 5, "returned wrong or size changed");
    int w3[] = {10, 30, 40, 50};
    CHECK("deque_remove of a middle node", deque_remove(&r, 20) == 1 && is(&r, w3, 4), "links wrong after removing 20 (size = %d)", r.size);
    int w4[] = {30, 40, 50};
    CHECK("deque_remove of the head", deque_remove(&r, 10) == 1 && is(&r, w4, 3), "links wrong after removing the head (size = %d)", r.size);
    int w5[] = {30, 40};
    CHECK("deque_remove of the tail", deque_remove(&r, 50) == 1 && is(&r, w5, 2), "links wrong after removing the tail (size = %d)", r.size);
    deque_remove(&r, 30);
    int w6[] = {40};
    CHECK("deque_remove down to a single node", is(&r, w6, 1), "links wrong with one node (size = %d)", r.size);
    CHECK("deque_remove of the only node leaves it empty", deque_remove(&r, 40) == 1 && r.head == NULL && r.tail == NULL && r.size == 0, "not empty");

    deque_init(&r);
    push_back(&r, 5); push_back(&r, 1); push_back(&r, 5); push_back(&r, 2);
    deque_remove(&r, 5);
    int w7[] = {1, 5, 2};
    CHECK("deque_remove takes only the first match", is(&r, w7, 3), "links or values wrong (size = %d)", r.size);
    deque_free(&r);

    Deque v;
    deque_init(&v);
    deque_reverse(&v);
    CHECK("reversing an empty deque is safe", v.head == NULL && v.tail == NULL && v.size == 0, "empty deque changed");
    for (int i = 1; i <= 4; i++) push_back(&v, i);
    deque_reverse(&v);
    int w8[] = {4, 3, 2, 1};
    CHECK("deque_reverse reverses 1,2,3,4 with correct links", is(&v, w8, 4), "reversed list is wrong (size = %d)", v.size);
    deque_reverse(&v);
    int w9[] = {1, 2, 3, 4};
    CHECK("reversing twice restores the order", is(&v, w9, 4), "list is wrong (size = %d)", v.size);
    deque_free(&v);
    CHECK("deque_free leaves the deque empty", v.head == NULL && v.tail == NULL && v.size == 0, "head/tail/size not reset");

    Deque big;
    deque_init(&big);
    int pushed = 1;
    for (int i = 0; i < 1000; i++) if (push_back(&big, i) != 0) pushed = 0;
    long total = 0;
    int popped = 0;
    while (pop_front(&big, &out) == 0) { total += out; popped++; }
    CHECK("1000 pushes then 1000 pops come out in order, none lost", pushed && popped == 1000 && total == 499500 && big.size == 0, "popped %d items, total %ld", popped, total);
    return 0;
}
