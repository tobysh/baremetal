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

// FLAWED: predictable tokens, ignores old_token, never expires.
const char *ss_login(SessionStore *ss, const char *user, long now, uint64_t rnd, const char *old_token) {
    (void)rnd; (void)old_token;
    for (int i = 0; i < MAX_SESS; i++) if (!ss->s[i].used) {
        Session *s = &ss->s[i];
        snprintf(s->token, sizeof s->token, "%016u", ++ss->counter);
        snprintf(s->user, sizeof s->user, "%s", user);
        s->created = s->last_seen = now; s->used = 1;
        return s->token;
    }
    return NULL;
}

int ss_check(SessionStore *ss, const char *token, long now, char *user_out, size_t cap) {
    (void)now;
    for (int i = 0; i < MAX_SESS; i++) if (ss->s[i].used && !strncmp(ss->s[i].token, token, strlen(token))) {
        if (cap) snprintf(user_out, cap, "%s", ss->s[i].user);
        return 1;
    }
    return 0;
}

void ss_logout(SessionStore *ss, const char *token) { (void)ss; (void)token; }

int main(void) {
    SessionStore ss; ss_init(&ss);
    printf("%s\n", ss_login(&ss, "alice", 0, 0x1234abcdULL, NULL));
    return 0;
}
