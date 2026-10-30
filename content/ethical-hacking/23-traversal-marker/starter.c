#include <stdio.h>
#include <stddef.h>
#include <string.h>

typedef struct { int status; const char *body; } FileReply;
typedef struct { FileReply (*fetch)(void *, const char *); void *ctx; } FileTarget;
int find_traversal(const FileTarget *target, const char *control, const char *expected_control,
                   const char *marker, const char *const *candidates, int n) {
    (void)target; (void)control; (void)expected_control; (void)marker; (void)candidates; (void)n;
    return -1;
}
