#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int owner_signed;
    int start_day, end_day;
    const char *scope[8];
    int nscope;
    const char *forbidden[4];
    int nforbidden;
} Roe;

enum { GO_AHEAD = 0, STOP_NO_PERMISSION = 1, STOP_OUT_OF_SCOPE = 2, STOP_OUTSIDE_WINDOW = 3, STOP_FORBIDDEN_TECHNIQUE = 4 };

int quiz_answer(int n) {
    static const int a[8] = { 0, 1, 0, 1, 0, 1, 1, 0 };
    return (n >= 0 && n < 8) ? a[n] : -1;
}

static int in_scope(const char *e, const char *t) {
    if (e[0] == '*' && e[1] == '.') {
        size_t el = strlen(e) - 1, tl = strlen(t);
        return tl > el && strcmp(t + tl - el, e + 1) == 0;
    }
    return strcmp(e, t) == 0;
}

int may_proceed(const Roe *r, const char *target, int day, const char *technique) {
    if (!r->owner_signed) return STOP_NO_PERMISSION;
    int ok = 0;
    for (int i = 0; i < r->nscope; i++) if (in_scope(r->scope[i], target)) ok = 1;
    if (!ok) return STOP_OUT_OF_SCOPE;
    if (day < r->start_day || day > r->end_day) return STOP_OUTSIDE_WINDOW;
    for (int i = 0; i < r->nforbidden; i++) if (strcmp(r->forbidden[i], technique) == 0) return STOP_FORBIDDEN_TECHNIQUE;
    return GO_AHEAD;
}

int main(void) {
    Roe r = { 1, 10, 20, { "*.lab.test" }, 1, { "dos" }, 1 };
    printf("%d\n", may_proceed(&r, "web.lab.test", 12, "portscan"));
    return 0;
}
