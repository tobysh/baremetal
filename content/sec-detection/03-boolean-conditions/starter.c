#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/* get_field() from the first lesson is available below. */

/* Finds `key` on a key=value line (values may be "quoted \"with\" spaces"). First occurrence wins. */
static int get_field(const char *line, const char *key, char *out, size_t cap) {
  const char *p = line; size_t kl = strlen(key);
  while (*p) {
    while (*p == ' ') p++;
    if (!*p) break;
    const char *k = p;
    while (*p && *p != '=' && *p != ' ') p++;
    size_t n = (size_t)(p - k), vl = 0;
    char v[1024];
    if (*p == '=') {
      p++;
      if (*p == '"') {
        p++;
        while (*p && *p != '"') { if (*p == '\\' && p[1]) p++; if (vl < sizeof v - 1) v[vl++] = *p; p++; }
        if (*p == '"') p++;
      } else while (*p && *p != ' ') { if (vl < sizeof v - 1) v[vl++] = *p; p++; }
    }
    v[vl] = 0;
    if (n == kl && memcmp(k, key, n) == 0) { snprintf(out, cap, "%s", v); return 1; }
  }
  return 0;
}

int main(void){puts("TODO");return 0;}
