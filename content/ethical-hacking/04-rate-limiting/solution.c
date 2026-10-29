#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define RL_KEYS 8
#define RL_MAX 16
typedef struct { char key[32]; long ts[RL_MAX]; int n; int used; } RlSlot;
typedef struct { int limit; long window; RlSlot slot[RL_KEYS]; } Limiter;

void rl_init(Limiter *l, int limit, long window) {
    memset(l, 0, sizeof *l);
    l->limit = limit; l->window = window;
}

static void expire(Limiter *l, RlSlot *s, long now) {
    int k = 0;
    for (int i = 0; i < s->n; i++) if (s->ts[i] > now - l->window) s->ts[k++] = s->ts[i];
    s->n = k;
    if (k == 0) s->used = 0;
}

static RlSlot *find(Limiter *l, const char *key, long now) {
    for (int i = 0; i < RL_KEYS; i++) if (l->slot[i].used) expire(l, &l->slot[i], now);
    for (int i = 0; i < RL_KEYS; i++) if (l->slot[i].used && !strcmp(l->slot[i].key, key)) return &l->slot[i];
    return NULL;
}

int rl_allow(Limiter *l, const char *key, long now) {
    RlSlot *s = find(l, key, now);
    if (!s) {
        for (int i = 0; i < RL_KEYS; i++) if (!l->slot[i].used) { s = &l->slot[i]; break; }
        if (!s) return 0;
        memset(s, 0, sizeof *s);
        strncpy(s->key, key, 31);
    }
    if (s->n >= l->limit) return 0;
    s->ts[s->n++] = now;
    s->used = 1;
    return 1;
}

long rl_retry_after(Limiter *l, const char *key, long now) {
    RlSlot *s = find(l, key, now);
    if (!s || s->n < l->limit) return 0;
    return s->ts[0] + l->window - now;
}

int main(void) {
    Limiter l;
    rl_init(&l, 3, 10);
    for (long t = 0; t < 6; t++) printf("t=%ld allowed=%d\n", t, rl_allow(&l, "198.51.100.7", t));
    return 0;
}
