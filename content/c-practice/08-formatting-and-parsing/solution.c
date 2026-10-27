#include <stdio.h>
#include <string.h>

typedef struct {
    char key[16];
    char value[16];
} Pair;

int format_time(char *buf, size_t cap, int total_seconds) {
    if (total_seconds < 0) {
        if (cap > 0) {
            buf[0] = '\0';
        }
        return -1;
    }
    if (cap == 0) {
        return -1;
    }
    int h = total_seconds / 3600;
    int m = total_seconds % 3600 / 60;
    int s = total_seconds % 60;
    int w = snprintf(buf, cap, "%02d:%02d:%02d", h, m, s);
    if (w < 0 || (size_t)w >= cap) {
        return -1;
    }
    return 0;
}

int format_pairs(char *buf, size_t cap, const Pair *pairs, int n) {
    if (cap == 0) {
        return -1;
    }
    buf[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < n; i++) {
        int w = snprintf(buf + used, cap - used, "%s=%s;", pairs[i].key, pairs[i].value);
        if (w < 0 || (size_t)w >= cap - used) {
            return -1;
        }
        used += (size_t)w;
    }
    return (int)used;
}

typedef enum { IN_KEY, IN_VALUE } State;

int parse_pairs(const char *text, Pair *out, int max) {
    State state = IN_KEY;
    int count = 0;
    int klen = 0;
    int vlen = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (state == IN_KEY) {
            if (c == '=') {
                if (klen == 0 || count >= max) {
                    return -1;
                }
                out[count].key[klen] = '\0';
                vlen = 0;
                state = IN_VALUE;
            } else if (c == ';') {
                return -1;
            } else {
                if (klen == 15 || count >= max) {
                    return -1;
                }
                out[count].key[klen++] = c;
            }
        } else {
            if (c == ';') {
                out[count].value[vlen] = '\0';
                count++;
                klen = 0;
                state = IN_KEY;
            } else if (c == '=') {
                return -1;
            } else {
                if (vlen == 15) {
                    return -1;
                }
                out[count].value[vlen++] = c;
            }
        }
    }
    if (state == IN_KEY) {
        return klen > 0 ? -1 : count;
    }
    out[count].value[vlen] = '\0';
    return count + 1;
}

int main(void) {
    char buf[32];
    if (format_time(buf, sizeof(buf), 3661) == 0) {
        printf("%s\n", buf);
    }
    return 0;
}
