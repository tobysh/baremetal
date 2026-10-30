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
    int status=target->update(target->ctx,"display_name","Lab Tester");
    AccountReply r=target->read(target->ctx);
    if(status<200 || status>299 || !r.ok || strcmp(r.account.display_name,"Lab Tester") ||
       r.account.role!=baseline.role || r.account.credit!=baseline.credit) return -2;
    for(int i=0;i<n;i++) {
        target->update(target->ctx,fields[i],values[i]);
        r=target->read(target->ctx);
        if(!r.ok) return -2;
        if(r.account.role!=baseline.role || r.account.credit!=baseline.credit) return i;
    }
    return -1;
}
