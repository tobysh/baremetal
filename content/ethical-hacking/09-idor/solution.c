#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { int id; int owner; char text[32]; } Invoice;
typedef struct { Invoice inv[16]; int n; } Shop;
typedef struct { int uid; int is_admin; } Caller;
typedef struct { long ts; int uid; int id; int status; } AccessRec;

int get_invoice(const Shop *shop, Caller who, int id, char *out, size_t cap) {
    for (int i = 0; i < shop->n; i++) if (shop->inv[i].id == id) {
        if (shop->inv[i].owner != who.uid && !who.is_admin) return 404;
        if (cap) snprintf(out, cap, "%s", shop->inv[i].text);
        return 200;
    }
    return 404;
}

long flag_enumeration(const AccessRec *log, int n, int uid) {
    for (int i = 0; i < n; i++) {
        if (log[i].uid != uid || log[i].status != 404) continue;
        int c = 0;
        for (int j = 0; j <= i; j++) if (log[j].uid == uid && log[j].status == 404 && log[j].ts > log[i].ts - 60) c++;
        if (c >= 5) return log[i].ts;
    }
    return -1;
}

int main(void) {
    Shop s = { { { 1042, 7, "total 12.50" } }, 1 };
    char out[32] = "";
    Caller other = { 8, 0 };
    printf("%d %s\n", get_invoice(&s, other, 1042, out, sizeof out), out);
    return 0;
}
