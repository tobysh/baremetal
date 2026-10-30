#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char title[96]; int severity; char root_cause[200]; char evidence[4][120]; int nevidence; char remediation[200]; } ReportIn;

int render_report(const ReportIn *in, char *out, size_t cap) {
    (void)in; (void)out; (void)cap;
    return -1;
}

int lint_report(const char *text) {
    (void)text;
    return 0;
}

int main(void) {
    ReportIn r = { "Invoice page shows other users' invoices", 3, "The handler never compares the owner with the caller.", { "GET /invoice/1043 as user 7 returned 200" }, 1, "Compare invoice.owner with the session user on every request." };
    char buf[1024];
    if (render_report(&r, buf, sizeof buf) > 0) fputs(buf, stdout);
    return 0;
}
