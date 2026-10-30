#include <stdio.h>
#include <stddef.h>
#include <string.h>

enum { INCONCLUSIVE = -1, ENFORCED = 0, BYPASS = 1 };
typedef struct { int status, effect; } Reply;
typedef struct { int owner, other, object, operation; } Probe;
typedef struct { Reply (*request)(void *, int, int, int); void *ctx; } AccessTarget;
int audit_access(const AccessTarget *target, const Probe *probes, int n, int *verdicts) {
    (void)target; (void)probes;
    for (int i = 0; i < n; i++) verdicts[i] = INCONCLUSIVE;
    return 0;
}
