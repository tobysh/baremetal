#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { URL_OK = 0, URL_SCHEME = 1, URL_MALFORMED = 2, URL_CREDENTIALS = 3, URL_NOT_ALLOWED = 4, URL_PORT = 5, URL_INTERNAL = 6, URL_NO_ADDRESS = 7 };
typedef uint32_t (*resolve_fn)(void *ctx, const char *host, int *ok);

// FLAWED: only checks that the text contains the allowed name somewhere.
int url_policy(const char *url, resolve_fn res, void *ctx, const char *const *allow, int nallow, uint32_t *pinned) {
    (void)res; (void)ctx; (void)pinned;
    for (int i = 0; i < nallow; i++) if (strstr(url, allow[i] + 2)) return URL_OK;
    return URL_NOT_ALLOWED;
}

int redirect_policy(const char *location, resolve_fn res, void *ctx, const char *const *allow, int nallow, uint32_t *pinned) {
    return url_policy(location, res, ctx, allow, nallow, pinned);
}

int main(void) {
    const char *allow[] = { "*.cdn-example.test" };
    printf("%d\n", url_policy("http://evil.test/?x=cdn-example.test", NULL, NULL, allow, 1, NULL));
    return 0;
}
