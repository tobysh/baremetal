#include <stdio.h>
#include <string.h>

#define CAP 4

typedef struct {
    int qty[CAP];
    int used;
    int total;
} cart;

typedef enum {
    DIAG_UNKNOWN = 0,
    DIAG_MISSING_CAPACITY_CHECK,   // a line is stored past the end of qty[]
    DIAG_COST_IGNORES_QUANTITY,    // total adds price instead of qty * price
    DIAG_COUNTER_NOT_RESET,        // used and total start with stale values
    DIAG_INCREMENT_TWICE           // used is increased more than once per call
} diagnosis_code;

void cart_init(cart *c) {
    memset(c, 0, sizeof *c);
}

// Returns 0 if stored, -1 if the cart is full (nothing changes).
int add_item(cart *c, int qty, int price) {
    // BUG: no capacity check; qty[CAP] is the `used` field
    c->qty[c->used] = qty;
    c->used++;
    c->total += price;
    return 0;
}

// Root cause of the corrupted `used` counter in call 5.
diagnosis_code diagnosis(void) {
    return DIAG_UNKNOWN;
}

int main(void) {
    cart c;
    cart_init(&c);
    int lines[5][2] = {{2, 10}, {3, 5}, {1, 20}, {4, 2}, {7, 3}};
    for (int i = 0; i < 5; i++) {
        add_item(&c, lines[i][0], lines[i][1]);
        printf("call %d: used=%d total=%d\n", i + 1, c.used, c.total);
    }
    return 0;
}
