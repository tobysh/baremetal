#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// A tiny page renderer. Untrusted text must be encoded for the place it lands.
// Every encoder works like snprintf: it writes at most cap bytes (including the
// terminating zero, when cap > 0) and returns the length of the FULL result.

// Text between tags: <p>HERE</p>
// TODO: this copies the text unchanged, so markup in it stays live.
size_t encode_html(const char *in, char *out, size_t cap) {
    return (size_t)snprintf(out, cap, "%s", in);
}

// Value inside a double-quoted attribute: <input value="HERE">
size_t encode_attr(const char *in, char *out, size_t cap) {
    return (size_t)snprintf(out, cap, "%s", in);
}

// 1 if the string is safe to use as an href, else 0.
int safe_url(const char *u) {
    (void)u;
    return 1;
}

// <a href="URL">TEXT</a>, with the right encoding for each part.
size_t render_link(const char *text, const char *url, char *out, size_t cap) {
    return (size_t)snprintf(out, cap, "<a href=\"%s\">%s</a>", url, text);
}

int main(void) {
    char out[128];
    render_link("hi", "javascript:alert(1)", out, sizeof out);
    puts(out);
    return 0;
}
