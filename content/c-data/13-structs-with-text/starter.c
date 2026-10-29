#include <stdio.h>
#include <string.h>

typedef struct {
    char name[16];
    int qty;
    int price_cents;
} Item;

Item item_make(char name[], int qty, int price_cents) {
    Item it = {"", 0, 0};
    return it;
}

int item_total(Item it) {
    return 0;
}

int inventory_value(Item items[], int n) {
    return 0;
}

int find_item(Item items[], int n, char name[]) {
    return -2;
}

int restock(Item items[], int n, char name[], int amount) {
    return -2;
}

// Optional: try them with Run.
int main(void) {
    Item shop[2];
    shop[0] = item_make("apple", 4, 50);
    shop[1] = item_make("a very long name indeed", 2, 125);
    printf("%s / %s\n", shop[0].name, shop[1].name);
    printf("value = %d (expected 450)\n", inventory_value(shop, 2));
    return 0;
}
