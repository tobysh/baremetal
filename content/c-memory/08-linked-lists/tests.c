/* Returns 1 if the list holds exactly the n values in expected, in order. */
static int list_equals(const Node *head, const int *expected, size_t n) {
    const Node *cur = head;
    for (size_t i = 0; i < n; i++) {
        if (cur == NULL || cur->value != expected[i]) return 0;
        cur = cur->next;
    }
    return cur == NULL;
}

int main(void) {
    Node *empty = NULL;
    CHECK("list_length(NULL) is 0", list_length(empty) == 0, "got %zu", list_length(empty));

    Node *one = push_front(NULL, 7);
    CHECK("push_front(NULL, 7) makes a one-node list holding 7", one != NULL && one->value == 7 && one->next == NULL, "bad node");
    CHECK("list_length of a one-node list is 1", list_length(one) == 1, "got %zu", list_length(one));
    free_list(one);

    Node *l = NULL;
    for (int i = 1; i <= 3; i++) l = push_front(l, i);
    int e321[] = {3, 2, 1};
    CHECK("pushing 1, 2, 3 gives the list 3 -> 2 -> 1", list_equals(l, e321, 3), "wrong order or length");
    CHECK("list_length after three pushes is 3", list_length(l) == 3, "got %zu", list_length(l));

    l = push_front(l, -50);
    int e4[] = {-50, 3, 2, 1};
    CHECK("push_front adds to the front and keeps the rest (-50 -> 3 -> 2 -> 1)", list_equals(l, e4, 4), "wrong order or length");

    l = reverse_list(l);
    int r4[] = {1, 2, 3, -50};
    CHECK("reverse_list turns -50,3,2,1 into 1,2,3,-50", list_equals(l, r4, 4), "wrong order or length");
    CHECK("reverse_list keeps the length at 4", list_length(l) == 4, "got %zu", list_length(l));
    l = reverse_list(l);
    CHECK("reversing twice restores the original order", list_equals(l, e4, 4), "wrong order or length");
    free_list(l);

    Node *none = reverse_list(NULL);
    CHECK("reverse_list(NULL) is NULL", none == NULL, "got a non-NULL pointer");

    Node *single = push_front(NULL, 5);
    Node *single_rev = reverse_list(single);
    CHECK("reverse_list of one node returns that node", single_rev == single && single_rev != NULL && single_rev->next == NULL, "bad result");
    free_list(single_rev);

    Node *big = NULL;
    for (int i = 0; i < 1000; i++) big = push_front(big, i);
    CHECK("list_length of 1000 pushes is 1000", list_length(big) == 1000, "got %zu", list_length(big));
    Node *big_rev = reverse_list(big);
    int ok = big_rev != NULL;
    int expect = 0;
    for (const Node *c = big_rev; ok && c != NULL; c = c->next, expect++) {
        if (c->value != expect) ok = 0;
    }
    CHECK("reversing 999..0 gives 0..999", ok && expect == 1000, "wrong order (checked %d nodes)", expect);
    free_list(big_rev);

    free_list(NULL);
    CHECK("free_list(NULL) does not crash", 1, "");
    return 0;
}
