int main(void) {
    Item a = item_make("apple", 4, 50);
    CHECK("item_make stores the name", strcmp(a.name, "apple") == 0, "got \"%s\"", a.name);
    CHECK("item_make stores qty and price", a.qty == 4 && a.price_cents == 50, "got %d and %d", a.qty, a.price_cents);
    Item l = item_make("0123456789ABCDEFGHIJ", 1, 1);
    CHECK("item_make cuts long names to 15 characters", strcmp(l.name, "0123456789ABCDE") == 0, "got \"%s\"", l.name);
    Item e15 = item_make("123456789012345", 1, 1);
    CHECK("item_make keeps a name of exactly 15 characters", strcmp(e15.name, "123456789012345") == 0, "got \"%s\"", e15.name);
    Item e = item_make("", 0, 0);
    CHECK("item_make accepts an empty name", e.name[0] == '\0', "got %d", e.name[0]);
    Item dirty;
    memset(&dirty, 'Z', sizeof dirty);
    dirty = item_make("hi", 2, 3);
    CHECK("item_make terminates the name", dirty.name[2] == '\0' && dirty.qty == 2 && dirty.price_cents == 3, "got %d", dirty.name[2]);

    CHECK("item_total multiplies qty by price", item_total(a) == 200, "got %d", item_total(a));
    CHECK("item_total of a zero quantity", item_total(item_make("x", 0, 999)) == 0, "got %d", item_total(item_make("x", 0, 999)));

    Item shop[4];
    shop[0] = item_make("apple", 4, 50);
    shop[1] = item_make("pear", 3, 70);
    shop[2] = item_make("fig", 10, 15);
    shop[3] = item_make("plum", 0, 80);
    CHECK("inventory_value sums all items", inventory_value(shop, 4) == 560, "got %d", inventory_value(shop, 4));
    CHECK("inventory_value respects n", inventory_value(shop, 2) == 410, "got %d", inventory_value(shop, 2));
    CHECK("inventory_value of an empty table is 0", inventory_value(shop, 0) == 0, "got %d", inventory_value(shop, 0));

    CHECK("find_item finds the first item", find_item(shop, 4, "apple") == 0, "got %d", find_item(shop, 4, "apple"));
    CHECK("find_item finds a middle item", find_item(shop, 4, "fig") == 2, "got %d", find_item(shop, 4, "fig"));
    CHECK("find_item returns -1 when absent", find_item(shop, 4, "kiwi") == -1, "got %d", find_item(shop, 4, "kiwi"));
    CHECK("find_item needs the whole name", find_item(shop, 4, "pea") == -1 && find_item(shop, 4, "pears") == -1, "prefix matched");
    CHECK("find_item respects n", find_item(shop, 2, "fig") == -1, "got %d", find_item(shop, 2, "fig"));

    int q = restock(shop, 4, "fig", 5);
    CHECK("restock returns the new quantity", q == 15 && shop[2].qty == 15, "got %d", q);
    q = restock(shop, 4, "plum", 7);
    CHECK("restock can fill an empty shelf", q == 7 && shop[3].qty == 7, "got %d", q);
    q = restock(shop, 4, "kiwi", 100);
    CHECK("restock of a missing item returns -1", q == -1, "got %d", q);
    CHECK("restock of a missing item changes nothing", shop[0].qty == 4 && shop[1].qty == 3 && shop[2].qty == 15 && shop[3].qty == 7, "quantities changed");
    return 0;
}
