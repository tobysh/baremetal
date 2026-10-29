#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int detect_v1(const char *sample) { return strstr(sample, "NET BEACON") != NULL; }

static int hexv(int c) { if (c >= '0' && c <= '9') return c - '0'; if (c >= 'a' && c <= 'f') return c - 'a' + 10; if (c >= 'A' && c <= 'F') return c - 'A' + 10; return -1; }

static int scan(const char *text, size_t len, int depth) {
    size_t i = 0;
    while (i < len) {
        size_t e = i; while (e < len && text[e] != '\n') e++;
        size_t s = i; while (s < e && (text[s] == ' ' || text[s] == '\t' || text[s] == '\r')) s++;
        size_t ll = e - s;
        i = e + 1;
        if (ll == 0 || text[s] == '#') continue;
        if (ll >= 9 && tolower((unsigned char)text[s]) == 'p' && tolower((unsigned char)text[s+1]) == 'a' && tolower((unsigned char)text[s+2]) == 'c' && tolower((unsigned char)text[s+3]) == 'k' && text[s+4] == ' ' && tolower((unsigned char)text[s+5]) == 'x' && tolower((unsigned char)text[s+6]) == 'o' && tolower((unsigned char)text[s+7]) == 'r' && text[s+8] == '=') {
            if (depth >= 3) continue;
            int a = ll > 10 ? hexv(text[s+9]) : -1, b = ll > 10 ? hexv(text[s+10]) : -1;
            if (a < 0 || b < 0) continue;
            int key = a * 16 + b;
            size_t p = s + 11; if (p < e && text[p] == ' ') p++;
            char buf[2100]; size_t n = 0;
            while (p + 1 < e && n < 2000) { int h = hexv(text[p]), l = hexv(text[p+1]); if (h < 0 || l < 0) break; buf[n++] = (char)((h * 16 + l) ^ key); p += 2; }
            if (scan(buf, n, depth + 1)) return 1;
            continue;
        }
        char norm[512]; size_t n = 0; int sp = 0;
        for (size_t k = s; k < e && n < sizeof norm - 1; k++) {
            char c = text[k];
            if (c == '"' || c == '+' || c == '\r') continue;
            if (c == ' ' || c == '\t') { sp = 1; continue; }
            if (sp && n) norm[n++] = ' ';
            sp = 0;
            norm[n++] = (char)tolower((unsigned char)c);
        }
        norm[n] = 0;
        if (!strcmp(norm, "net beacon") || !strncmp(norm, "net beacon ", 11)) return 1;
    }
    return 0;
}

int detect_v2(const char *sample) { return scan(sample, strlen(sample), 0); }

int main(void) {
    printf("%d\n", detect_v1("NET BEACON update.example.test"));
    return 0;
}
