#include <stdio.h>
#include <stddef.h>
#include <string.h>

enum { INCONCLUSIVE = -1, ENFORCED = 0, BYPASS = 1 };
typedef struct { int status, effect; } Reply;
typedef struct { int owner, other, object, operation; } Probe;
typedef struct { Reply (*request)(void *, int, int, int); void *ctx; } AccessTarget;
static int succeeded(Reply r) { return r.status >= 200 && r.status <= 299 && r.effect != 0; }
int audit_access(const AccessTarget *target, const Probe *probes, int n, int *verdicts) {
    int found = 0;
    for (int i = 0; i < n; i++) {
        Probe p = probes[i];
        Reply control = target->request(target->ctx, p.owner, p.object, p.operation);
        verdicts[i] = INCONCLUSIVE;
        if (!succeeded(control)) continue;
        Reply replay = target->request(target->ctx, p.other, p.object, p.operation);
        verdicts[i] = succeeded(replay) ? BYPASS : ENFORCED;
        found += verdicts[i] == BYPASS;
    }
    return found;
}
