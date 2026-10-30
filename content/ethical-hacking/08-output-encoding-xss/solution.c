#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Append-with-truncation helper: tracks the full length, writes only what fits.
typedef struct { char *out; size_t cap; size_t len; } Sink;
static void put(Sink *s, char c) {
    if (s->cap && s->len + 1 < s->cap) s->out[s->len] = c;
    s->len++;
}
static void puts_(Sink *s, const char *t) { while (*t) put(s, *t++); }
static void finish(Sink *s) { if (s->cap) s->out[s->len < s->cap ? s->len : s->cap - 1] = 0; }

static size_t html_to(Sink *s, const char *in) {
    for (; *in; in++) switch (*in) {
    case '&': puts_(s, "&amp;"); break;
    case '<': puts_(s, "&lt;"); break;
    case '>': puts_(s, "&gt;"); break;
    case '"': puts_(s, "&quot;"); break;
    case '\'': puts_(s, "&#39;"); break;
    default: put(s, *in);
    }
    return s->len;
}

static void attr_to(Sink *s, const char *in) {
    static const char hex[] = "0123456789ABCDEF";
    for (; *in; in++) {
        unsigned char c = (unsigned char)*in;
        if (isalnum(c) && c < 0x80) put(s, (char)c);
        else if (c == '.' || c == ',' || c == '-' || c == '_' || c >= 0x80) put(s, (char)c);
        else { puts_(s, "&#x"); put(s, hex[c >> 4]); put(s, hex[c & 15]); put(s, ';'); }
    }
}

size_t encode_html(const char *in, char *out, size_t cap) {
    Sink s = { out, cap, 0 };
    html_to(&s, in); finish(&s);
    return s.len;
}

size_t encode_attr(const char *in, char *out, size_t cap) {
    Sink s = { out, cap, 0 };
    attr_to(&s, in); finish(&s);
    return s.len;
}

int safe_url(const char *u) {
    if (!u || !*u) return 0;
    for (const char *p = u; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c <= 0x20 || c == 0x7f || c == '\\') return 0;
    }
    if (u[0] == '/' && u[1] == '/') return 0;
    const char *p = u;
    while (*p && *p != ':' && *p != '/' && *p != '?' && *p != '#') p++;
    if (*p != ':') return 1;  // relative reference
    size_t n = (size_t)(p - u);
    static const char *ok[] = { "http", "https", "mailto" };
    for (int i = 0; i < 3; i++) {
        if (strlen(ok[i]) != n) continue;
        size_t j = 0; while (j < n && tolower((unsigned char)u[j]) == ok[i][j]) j++;
        if (j == n) return 1;
    }
    return 0;
}

size_t render_link(const char *text, const char *url, char *out, size_t cap) {
    Sink s = { out, cap, 0 };
    puts_(&s, "<a href=\"");
    if (safe_url(url)) attr_to(&s, url); else puts_(&s, "#");
    puts_(&s, "\">");
    html_to(&s, text);
    puts_(&s, "</a>");
    finish(&s);
    return s.len;
}

int main(void) {
    char out[128];
    render_link("hi", "javascript:alert(1)", out, sizeof out);
    puts(out);
    return 0;
}
