#include <stdio.h>
#include <stddef.h>
#include <string.h>

typedef struct { char display_name[32]; int role, credit; } Account;
typedef struct { int ok; Account account; } AccountReply;
typedef struct {
    int (*update)(void *, const char *, const char *);
    AccountReply (*read)(void *);
    void *ctx;
} ProfileTarget;
int probe_fields(const ProfileTarget *target, Account baseline,
                 const char *const *fields, const char *const *values, int n) {
    (void)target; (void)baseline; (void)fields; (void)values; (void)n;
    return -1;
}
