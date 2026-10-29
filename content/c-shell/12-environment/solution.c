#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int name_start(int c) { return isalpha(c) || c == '_'; }
static int name_char(int c) { return isalnum(c) || c == '_'; }

static int valid_name(const char *s, size_t len) {
    if (len == 0 || !name_start((unsigned char)s[0])) return 0;
    for (size_t i = 1; i < len; i++)
        if (!name_char((unsigned char)s[i])) return 0;
    return 1;
}

struct out {
    char *buf;
    size_t size, len;
    int ok;
};

static void add(struct out *o, const char *s, size_t n) {
    if (!o->ok) return;
    if (o->len + n + 1 > o->size) { o->ok = 0; return; }
    memcpy(o->buf + o->len, s, n);
    o->len += n;
    o->buf[o->len] = '\0';
}

int expand_word(const char *in, char *out, size_t out_size, int last_status) {
    struct out o = {out, out_size, 0, out_size > 0};
    if (out_size > 0) out[0] = '\0';
    for (size_t i = 0; in[i] && o.ok;) {
        if (in[i] != '$') { add(&o, &in[i++], 1); continue; }
        char c = in[i + 1];
        if (c == '?') {
            char num[16];
            int k = snprintf(num, sizeof num, "%d", last_status);
            add(&o, num, (size_t)k);
            i += 2;
        } else if (c == '{') {
            const char *close = strchr(in + i + 2, '}');
            size_t len = close ? (size_t)(close - (in + i + 2)) : 0;
            if (close && valid_name(in + i + 2, len) && len < 256) {
                char name[256];
                memcpy(name, in + i + 2, len);
                name[len] = '\0';
                const char *v = getenv(name);
                if (v) add(&o, v, strlen(v));
                i = (size_t)(close - in) + 1;
            } else {
                add(&o, &in[i++], 1);
            }
        } else if (name_start((unsigned char)c)) {
            size_t len = 0;
            while (name_char((unsigned char)in[i + 1 + len])) len++;
            if (len < 256) {
                char name[256];
                memcpy(name, in + i + 1, len);
                name[len] = '\0';
                const char *v = getenv(name);
                if (v) add(&o, v, strlen(v));
            }
            i += 1 + len;
        } else {
            add(&o, &in[i++], 1);
        }
    }
    return o.ok ? (int)o.len : -1;
}

int do_export(const char *assignment) {
    const char *eq = strchr(assignment, '=');
    if (!eq) return 1;
    size_t len = (size_t)(eq - assignment);
    if (!valid_name(assignment, len) || len >= 256) return 1;
    char name[256];
    memcpy(name, assignment, len);
    name[len] = '\0';
    return setenv(name, eq + 1, 1) == 0 ? 0 : 1;
}

int do_unset(const char *name) {
    if (!valid_name(name, strlen(name))) return 1;
    return unsetenv(name) == 0 ? 0 : 1;
}

int main(void) {
    do_export("GREETING=hello");
    char out[128];
    int n = expand_word("[$GREETING] ${GREETING}world status=$? $", out, sizeof out, 3);
    printf("%d: %s\n", n, out);
    return 0;
}
