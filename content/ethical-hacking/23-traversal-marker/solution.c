#include <stdio.h>
#include <stddef.h>
#include <string.h>

typedef struct { int status; const char *body; } FileReply;
typedef struct { FileReply (*fetch)(void *, const char *); void *ctx; } FileTarget;
int find_traversal(const FileTarget *target, const char *control, const char *expected_control,
                   const char *marker, const char *const *candidates, int n) {
    FileReply r = target->fetch(target->ctx, control);
    if (r.status != 200 || !r.body || strcmp(r.body, expected_control)) return -2;
    for (int i=0; i<n; i++) {
        r = target->fetch(target->ctx, candidates[i]);
        if (r.status == 200 && r.body && !strcmp(r.body, marker)) return i;
    }
    return -1;
}
