#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* Per-client record: the distinct targets seen and the outcome counts for them. */
typedef struct {
    char ip[64];
    int n, o, c, f;        /* targets, open, closed, filtered */
    char sv[256][64];      /* server of each target */
    int pt[256];           /* port of each target */
} Client;

static Client cl[256];

int main(void)
{
    char line[1024];
    int nc = 0;

    while (fgets(line, sizeof line, stdin)) {
        char client[64], server[64], outcome[32], extra[2];
        long t;
        int port;

        /* Exactly five fields; a sixth makes the sscanf count 6 and the line is skipped. */
        if (sscanf(line, "%ld %63s %63s %d %31s %1s", &t, client, server, &port, outcome, extra) != 5)
            continue;
        if (port < 1 || port > 65535)
            continue;

        int kind;                              /* 0 open, 1 closed, 2 filtered */
        if (!strcmp(outcome, "open")) kind = 0;
        else if (!strcmp(outcome, "closed")) kind = 1;
        else if (!strcmp(outcome, "filtered")) kind = 2;
        else continue;

        int j = 0;
        while (j < nc && strcmp(cl[j].ip, client))
            j++;
        if (j == nc) {
            if (nc == 256)
                continue;
            strcpy(cl[nc++].ip, client);
        }

        /* A target is counted once, with its first outcome. */
        Client *x = &cl[j];
        int dup = 0;
        for (int i = 0; i < x->n; i++)
            if (x->pt[i] == port && !strcmp(x->sv[i], server))
                dup = 1;
        if (dup || x->n == 256)
            continue;
        strcpy(x->sv[x->n], server);
        x->pt[x->n++] = port;
        if (kind == 0) x->o++;
        else if (kind == 1) x->c++;
        else x->f++;
    }

    /* Collect flagged clients, then insertion-sort by count descending and name ascending. */
    int ord[256], m = 0;
    for (int i = 0; i < nc; i++)
        if (cl[i].n >= 5 && cl[i].c + cl[i].f > cl[i].o)
            ord[m++] = i;
    for (int i = 1; i < m; i++) {
        int v = ord[i], j = i;
        while (j && (cl[ord[j - 1]].n < cl[v].n ||
                     (cl[ord[j - 1]].n == cl[v].n && strcmp(cl[ord[j - 1]].ip, cl[v].ip) > 0))) {
            ord[j] = ord[j - 1];
            j--;
        }
        ord[j] = v;
    }

    if (!m)
        puts("no scans detected");
    for (int i = 0; i < m; i++) {
        Client *x = &cl[ord[i]];
        printf("%s targets=%d open=%d closed=%d filtered=%d\n", x->ip, x->n, x->o, x->c, x->f);
    }
    return 0;
}
