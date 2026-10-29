static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
static const char *SEVN[5] = { "Info", "Low", "Medium", "High", "Critical" };
static const char *TITLES[] = { "Invoice page shows other users' invoices", "Session token never expires", "Fetch feature can reach internal hosts", "Login accepts unlimited guesses", "Service binary is writable by all" };
static const char *CAUSES[] = { "The handler never compares the invoice owner with the caller.", "The store has no idle or absolute timeout.", "The URL is not checked against an allow-list after resolution.", "No rate limit is applied per account or per source.", "Mode 0777 on the file that root runs at boot." };
static const char *EVID[] = { "GET /invoice/1043 as user 7 returned 200 with another owner's data", "Token 0000000000000007 still valid 9 hours after login", "Request to http://img.cdn-example.test returned an internal address 10.1.2.3", "412 failed logins from 203.0.113.9 in 60 s, no block", "ls output: -rwxrwxrwx root root /opt/jobs/nightly.sh" };
static const char *REMS[] = { "Compare invoice.owner with the session user on every request and return 404 otherwise.", "Enforce a 300 second idle and 3600 second absolute timeout on the server.", "Resolve once, reject internal addresses and connect to the validated address.", "Add a sliding-window limit of 5 attempts per minute per account.", "Change the mode to 0755 and make root the owner." };

static void build(char *out, size_t cap, int sev, int t, int nev, int secret, int bits) {
    // bits: 1 no title, 2 bad severity, 4 no root cause, 8 no evidence, 16 weak remediation, 64 swapped order
    char o[2000] = ""; size_t n = 0;
    char title[200], sevs[40], cause[200], rem[200];
    snprintf(title, sizeof title, "%s", (bits & 1) ? "" : TITLES[t]);
    snprintf(sevs, sizeof sevs, "%s", (bits & 2) ? ((rnd() % 2) ? "critical" : "Severe") : SEVN[sev]);
    snprintf(cause, sizeof cause, "%s", (bits & 4) ? "   " : CAUSES[t]);
    snprintf(rem, sizeof rem, "%s", (bits & 16) ? ((rnd() % 2) ? "Fix it" : "tbd") : REMS[t]);
    char part[5][800];
    snprintf(part[0], sizeof part[0], "FINDING: %s\n", title);
    snprintf(part[1], sizeof part[1], "SEVERITY:  %s \n", sevs);
    snprintf(part[2], sizeof part[2], "ROOT CAUSE: %s\n", cause);
    snprintf(part[3], sizeof part[3], "EVIDENCE:\n");
    if (!(bits & 8)) for (int i = 0; i < nev; i++) {
        char b[300];
        if (secret && i == 0) snprintf(b, sizeof b, "- %s (login used password=hunter%d)\n", EVID[(t + i) % 5], (int)(rnd() % 90));
        else snprintf(b, sizeof b, "- %s\n", EVID[(t + i) % 5]);
        strcat(part[3], b);
    }
    snprintf(part[4], sizeof part[4], "REMEDIATION: %s\n", rem);
    int ord[5] = { 0, 1, 2, 3, 4 };
    if (bits & 64) { int a = 1 + (int)(rnd() % 3); int tmp = ord[a]; ord[a] = ord[a + 1]; ord[a + 1] = tmp; }
    for (int i = 0; i < 5; i++) { size_t l = strlen(part[ord[i]]); memcpy(o + n, part[ord[i]], l); n += l; o[n] = 0; }
    snprintf(out, cap, "%s", o);
}

int main(void) {
    {
        ReportIn r; memset(&r, 0, sizeof r);
        snprintf(r.title, sizeof r.title, "Session token never expires"); r.severity = 3;
        snprintf(r.root_cause, sizeof r.root_cause, "No timeout."); r.nevidence = 2;
        snprintf(r.evidence[0], 120, "token valid after 9 h"); snprintf(r.evidence[1], 120, "no expiry field");
        snprintf(r.remediation, sizeof r.remediation, "Add idle and absolute timeouts.");
        char out[512]; memset(out, '#', sizeof out);
        int n = render_report(&r, out, sizeof out);
        const char *want = "FINDING: Session token never expires\nSEVERITY: High\nROOT CAUSE: No timeout.\nEVIDENCE:\n- token valid after 9 h\n- no expiry field\nREMEDIATION: Add idle and absolute timeouts.\n";
        CHECK("report text has the exact structure", n == (int)strlen(want) && !strcmp(out, want), "got %d chars:\n%s", n, n > 0 ? out : "(nothing)");
        CHECK("a rendered report passes the linter", lint_report(out) == 0, "lint_report returned %d for a good report", lint_report(out));
    }
    {
        int bad = 0; char first[200] = "";
        for (int it = 0; it < 60; it++) {
            rs = 40 + it * 3;
            ReportIn r; memset(&r, 0, sizeof r);
            int t = (int)(rnd() % 5); r.severity = (int)(rnd() % 7) - 1; r.nevidence = (int)(rnd() % 5);
            snprintf(r.title, sizeof r.title, "%s", TITLES[t]); snprintf(r.root_cause, sizeof r.root_cause, "%s", CAUSES[t]); snprintf(r.remediation, sizeof r.remediation, "%s", REMS[t]);
            for (int i = 0; i < r.nevidence; i++) snprintf(r.evidence[i], 120, "%s", EVID[(t + i) % 5]);
            char want[2000]; int p = snprintf(want, sizeof want, "FINDING: %s\nSEVERITY: %s\nROOT CAUSE: %s\nEVIDENCE:\n", r.title, (r.severity >= 0 && r.severity <= 4) ? SEVN[r.severity] : "Info", r.root_cause);
            for (int i = 0; i < r.nevidence; i++) p += snprintf(want + p, sizeof want - (size_t)p, "- %s\n", r.evidence[i]);
            p += snprintf(want + p, sizeof want - (size_t)p, "REMEDIATION: %s\n", r.remediation);
            char out[2000]; int n = render_report(&r, out, sizeof out);
            if (n != p || strcmp(out, want)) { if (!bad) snprintf(first, sizeof first, "case %d: severity %d, %d bullets: length %d vs %d", it, r.severity, r.nevidence, n, p); bad++; }
        }
        CHECK("rendering matches for varied findings and severities", bad == 0, "%d of 60 differ, e.g. %s", bad, first);
    }
    {
        ReportIn r; memset(&r, 0, sizeof r);
        snprintf(r.title, sizeof r.title, "Odd name\nSEVERITY: Info\r\nFINDING: forged"); r.severity = 4;
        snprintf(r.root_cause, sizeof r.root_cause, "cause"); r.nevidence = 1; snprintf(r.evidence[0], 120, "line\nREMEDIATION: nothing"); snprintf(r.remediation, sizeof r.remediation, "Change the thing now please.");
        char out[600]; int n = render_report(&r, out, sizeof out);
        int lines = 0; for (char *q = out; *q; q++) if (*q == '\n') lines++;
        CHECK("line breaks inside values cannot forge sections", n > 0 && lines == 6 && strstr(out, "Odd name SEVERITY: Info  FINDING: forged") && strstr(out, "- line REMEDIATION: nothing"), "a report with five parts and one bullet has 6 lines, got %d", lines);
    }
    {
        ReportIn r; memset(&r, 0, sizeof r);
        snprintf(r.title, sizeof r.title, "T"); snprintf(r.root_cause, sizeof r.root_cause, "C"); snprintf(r.remediation, sizeof r.remediation, "R");
        char out[200]; int full = render_report(&r, out, sizeof out);
        int bad = 0;
        for (size_t cap = 0; cap <= (size_t)full + 1; cap++) {
            unsigned char blk[260]; memset(blk, 0xA5, sizeof blk);
            int n = render_report(&r, (char *)blk + 10, cap);
            int ok = (cap >= (size_t)full + 1) ? (n == full) : (n == -1 && (cap == 0 || blk[10] == 0));
            for (int i = 0; i < 10; i++) if (blk[i] != 0xA5) ok = 0;
            for (size_t i = cap; i < 200; i++) if (blk[10 + i] != 0xA5) ok = 0;
            if (!ok) bad++;
        }
        CHECK("a too-small buffer gives -1 and is never overrun", full > 0 && bad == 0, "%d capacities from 0 to %d behaved wrongly", bad, full + 1);
    }
    {
        int bad = 0; char first[200] = ""; int tot = 0;
        for (int it = 0; it < 400; it++) {
            rs = 900 + it * 11;
            int bits = 0; if (rnd() % 3 == 0) bits |= 1; if (rnd() % 3 == 0) bits |= 2; if (rnd() % 3 == 0) bits |= 4; if (rnd() % 3 == 0) bits |= 8; if (rnd() % 3 == 0) bits |= 16; if (rnd() % 4 == 0) bits |= 64;
            int secret = (rnd() % 4 == 0) && !(bits & 8);
            int t = (int)(rnd() % 5), sev = (int)(rnd() % 5), nev = 1 + (int)(rnd() % 3);
            char txt[3000]; build(txt, sizeof txt, sev, t, nev, secret, bits);
            int want = (bits & 1) | (bits & 2) | (bits & 4) | (bits & 8) | (bits & 16) | (secret ? 32 : 0) | (bits & 64);
            int got = lint_report(txt); tot++;
            if (got != want) { if (!bad) snprintf(first, sizeof first, "expected %d, got %d for:\n%.300s", want, got, txt); bad++; }
        }
        CHECK("the linter reports exactly the problems present", bad == 0, "%d of %d reports misjudged, e.g. %s", bad, tot, first);
    }
    {
        const char *redacted = "FINDING: Weak login\nSEVERITY: High\nROOT CAUSE: No limit on guesses.\nEVIDENCE:\n- login attempt password=[redacted] accepted\n- token=[redacted] reused\nREMEDIATION: Add a per-account attempt limit.\n";
        const char *missing = "SEVERITY: High\n";
        const char *empty = "";
        const char *twice = "FINDING: A\nFINDING: B\nSEVERITY: Low\nROOT CAUSE: x\nEVIDENCE:\n- e\nREMEDIATION: Change the setting to off.\n";
        CHECK("redacted secrets are fine", lint_report(redacted) == 0, "got %d", lint_report(redacted));
        CHECK("missing sections are all reported", lint_report(missing) == (1 | 4 | 8 | 16) && lint_report(empty) == (1 | 2 | 4 | 8 | 16), "got %d and %d (expect 29 and 31)", lint_report(missing), lint_report(empty));
        CHECK("a repeated label does not break the order check", lint_report(twice) == 0, "got %d", lint_report(twice));
    }
    return 0;
}
