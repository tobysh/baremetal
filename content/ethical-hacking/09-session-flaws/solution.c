#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_SESS 8
typedef struct { char token[17]; char user[24]; long created, last_seen; int used; } Session;
typedef struct { Session s[MAX_SESS]; unsigned counter; } SessionStore;

void ss_init(SessionStore *ss) { memset(ss, 0, sizeof *ss); }

static void purge(SessionStore *ss, long now) {
    for (int i = 0; i < MAX_SESS; i++)
        if (ss->s[i].used && (now - ss->s[i].last_seen > 300 || now - ss->s[i].created > 3600)) ss->s[i].used = 0;
}
static Session *find(SessionStore *ss, const char *token) {
    if (!token || strlen(token) != 16) return NULL;
    for (int i = 0; i < MAX_SESS; i++) if (ss->s[i].used && !strcmp(ss->s[i].token, token)) return &ss->s[i];
    return NULL;
}

const char *ss_login(SessionStore *ss, const char *user, long now, uint64_t rnd, const char *old_token) {
    purge(ss, now);
    Session *o = find(ss, old_token);
    if (o) o->used = 0;
    char tok[17];
    snprintf(tok, sizeof tok, "%016llx", (unsigned long long)rnd);
    if (find(ss, tok)) return NULL;
    for (int i = 0; i < MAX_SESS; i++) if (!ss->s[i].used) {
        Session *s = &ss->s[i];
        memcpy(s->token, tok, sizeof tok);
        snprintf(s->user, sizeof s->user, "%s", user);
        s->created = s->last_seen = now; s->used = 1;
        return s->token;
    }
    return NULL;
}

int ss_check(SessionStore *ss, const char *token, long now, char *user_out, size_t cap) {
    purge(ss, now);
    Session *s = find(ss, token);
    if (!s) return 0;
    s->last_seen = now;
    if (cap) snprintf(user_out, cap, "%s", s->user);
    return 1;
}

void ss_logout(SessionStore *ss, const char *token) {
    Session *s = find(ss, token);
    if (s) s->used = 0;
}

int main(void) {
    SessionStore ss; ss_init(&ss);
    printf("%s\n", ss_login(&ss, "alice", 0, 0x1234abcdULL, NULL));
    return 0;
}
