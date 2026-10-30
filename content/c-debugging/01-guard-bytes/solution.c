#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned char guard_lo[4];
    char          name[8];
    unsigned char guard_hi[4];
} record;

void record_init(record *r) {
    memset(r->guard_lo, 0xAA, sizeof r->guard_lo);
    memset(r->name, 0, sizeof r->name);
    memset(r->guard_hi, 0xAA, sizeof r->guard_hi);
}

// Store s in r->name. Returns how many characters did not fit.
int set_name(record *r, const char *s) {
    size_t room = sizeof r->name - 1;
    size_t i = 0;
    for (; i < room && s[i] != '\0'; i++) {
        r->name[i] = s[i];
    }
    r->name[i] = '\0';
    return (int)strlen(s + i);
}

static void dump(const record *r) {
    const unsigned char *p = (const unsigned char *)r;
    for (size_t i = 0; i < sizeof *r; i++) {
        printf("%02X%c", p[i], i + 1 == sizeof *r ? '\n' : ' ');
    }
}

// Run this to see the same kind of dump as in the lesson.
int main(void) {
    record r;
    record_init(&r);
    dump(&r);
    set_name(&r, "Bart");
    dump(&r);
    return 0;
}
