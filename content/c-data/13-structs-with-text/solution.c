#include <stdio.h>
#include <string.h>

typedef struct {
    char name[16];
    int qty;
    int price_cents;
} Item;

Item item_make(char name[], int qty, int price_cents) {
    Item it;
    int i = 0;
    while (name[i] != '\0' && i < 15) {
        it.name[i] = name[i];
        i++;
    }
    it.name[i] = '\0';
    it.qty = qty;
    it.price_cents = price_cents;
    return it;
}

int item_total(Item it) {
    return it.qty * it.price_cents;
}

int inventory_value(Item items[], int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += item_total(items[i]);
    }
    return total;
}

int find_item(Item items[], int n, char name[]) {
    for (int i = 0; i < n; i++) {
        if (strcmp(items[i].name, name) == 0) return i;
    }
    return -1;
}

int restock(Item items[], int n, char name[], int amount) {
    int i = find_item(items, n, name);
    if (i < 0) return -1;
    items[i].qty += amount;
    return items[i].qty;
}

int main(void) {
    Item shop[2];
    shop[0] = item_make("apple", 4, 50);
    shop[1] = item_make("a very long name indeed", 2, 125);
    printf("%s / %s\n", shop[0].name, shop[1].name);
    printf("value = %d\n", inventory_value(shop, 2));
    return 0;
}
