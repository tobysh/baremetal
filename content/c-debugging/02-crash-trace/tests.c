static item chain[3] = {
    {"pen",     120, &chain[1]},
    {"ink",     450, &chain[2]},
    {"stapler", 999, NULL},
};
static item two[2] = {
    {"pen", 120, &two[1]},
    {"ink", 450, NULL},
};
static item free_item[1] = {
    {"sample", 0, NULL},
};

int main(void) {
    CHECK("a known item has its price", price_of(two, "ink") == 450, "price_of(ink) = %d, expected 450", price_of(two, "ink"));
    CHECK("the first item is found", price_of(two, "pen") == 120, "price_of(pen) = %d, expected 120", price_of(two, "pen"));
    CHECK("a free item costs 0, not -1", price_of(free_item, "sample") == 0, "price_of(sample) = %d, expected 0", price_of(free_item, "sample"));
    CHECK("an unknown item gives -1", price_of(two, "stapler") == -1, "price_of(stapler) = %d, expected -1", price_of(two, "stapler"));
    CHECK("an empty list gives -1", price_of(NULL, "pen") == -1, "price_of(NULL, pen) = %d, expected -1", price_of(NULL, "pen"));

    const char *all[] = {"pen", "ink", "stapler"};
    int t = order_total(chain, all, 3);
    CHECK("a fully known order is summed", t == 1569, "total %d, expected 1569", t);

    const char *last[] = {"pen", "ink", "stapler"};
    t = order_total(two, last, 3);
    CHECK("the bug report's order no longer crashes", t == 570, "total %d, expected 570", t);

    const char *first[] = {"glue", "pen", "ink"};
    t = order_total(two, first, 3);
    CHECK("an unknown name first is skipped", t == 570, "total %d, expected 570", t);

    const char *mid[] = {"pen", "tape", "ink"};
    t = order_total(two, mid, 3);
    CHECK("an unknown name in the middle is skipped", t == 570, "total %d, expected 570", t);

    const char *none[] = {"glue", "tape"};
    t = order_total(two, none, 2);
    CHECK("an order of only unknown names is 0", t == 0, "total %d, expected 0", t);

    const char *dup[] = {"pen", "pen", "ink"};
    t = order_total(two, dup, 3);
    CHECK("a repeated name is counted twice", t == 690, "total %d, expected 690", t);

    t = order_total(two, all, 0);
    CHECK("zero names total 0", t == 0, "total %d, expected 0", t);

    t = order_total(NULL, all, 3);
    CHECK("an empty list totals 0", t == 0, "total %d, expected 0", t);

    const char *fr[] = {"sample", "sample"};
    t = order_total(free_item, fr, 2);
    CHECK("free items add 0 and are not mistaken for missing", t == 0, "total %d, expected 0", t);
    return 0;
}
