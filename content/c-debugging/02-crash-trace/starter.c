#include <stdio.h>
#include <string.h>

typedef struct item {
    const char  *name;
    int          cents;
    struct item *next;
} item;

static const item *find_item(const item *list, const char *name) {
    for (; list != NULL; list = list->next) {
        if (strcmp(list->name, name) == 0) return list;
    }
    return NULL;
}

// Price in cents, or -1 if there is no such item.
int price_of(const item *list, const char *name) {
    const item *it = find_item(list, name);
    return it->cents;
}

// Sum of the prices of the n names; unknown names are skipped.
int order_total(const item *list, const char *const *names, int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += price_of(list, names[i]);
    }
    return total;
}

int main(void) {
    item stapler_less[2] = {
        {"pen", 120, &stapler_less[1]},
        {"ink", 450, NULL},
    };
    const char *order[] = {"pen", "ink", "stapler"};
    printf("%d\n", order_total(stapler_less, order, 3));
    return 0;
}
