#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// An engagement: the paperwork in code form.
typedef struct {
    int owner_signed;          // 1 if the owner (or their authorised delegate) signed in writing
    int start_day, end_day;    // test window, inclusive
    const char *scope[8];      // exact hosts, or "*.suffix" wildcards
    int nscope;
    const char *forbidden[4];  // forbidden techniques
    int nforbidden;
} Roe;

enum { GO_AHEAD = 0, STOP_NO_PERMISSION = 1, STOP_OUT_OF_SCOPE = 2, STOP_OUTSIDE_WINDOW = 3, STOP_FORBIDDEN_TECHNIQUE = 4 };

// Part 1: return 1 for true, 0 for false (statements 0..7 in the task).
int quiz_answer(int n) {
    (void)n;
    return -1;
}

// Part 2: should the tester proceed?
int may_proceed(const Roe *r, const char *target, int day, const char *technique) {
    (void)r; (void)target; (void)day; (void)technique;
    return GO_AHEAD;
}

int main(void) {
    Roe r = { 1, 10, 20, { "*.lab.test" }, 1, { "dos" }, 1 };
    printf("%d\n", may_proceed(&r, "web.lab.test", 12, "portscan"));
    return 0;
}
