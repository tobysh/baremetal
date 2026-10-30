typedef struct { int call, bad, size; Probe *p; Reply *a, *b; int replay; } Matrix;
static Reply matrix_request(void *v, int uid, int obj, int op) {
    Matrix *m = v;
    if (m->call >= m->size) { m->bad++; return (Reply){500,0}; }
    Probe p = m->p[m->call];
    if (uid != (m->replay ? p.other : p.owner) || obj != p.object || op != p.operation) m->bad++;
    Reply r = m->replay ? m->b[m->call] : m->a[m->call];
    if (m->replay || !(r.status >= 200 && r.status <= 299 && r.effect)) { m->call++; m->replay=0; }
    else m->replay=1;
    return r;
}
int main(void) {
    int bad=0, order=0;
    for (int seed=0; seed<37; seed++) {
        Probe p[25]; Reply a[25], b[25]; int out[27], want[25], total=0;
        int codes[]={199,200,204,299,300,403,404,500};
        for (int i=0;i<25;i++) {
            p[i]=(Probe){10+seed,200+seed,1000+i*7+seed,i%3};
            a[i]=(Reply){codes[(i+seed)%8],(i+seed)%3};
            b[i]=(Reply){codes[(i*3+seed)%8],(i+seed+1)%3};
            int control=a[i].status>=200 && a[i].status<=299 && a[i].effect;
            want[i]=!control ? INCONCLUSIVE : (b[i].status>=200 && b[i].status<=299 && b[i].effect ? BYPASS : ENFORCED);
            total+=want[i]==BYPASS;
        }
        for(int i=0;i<27;i++) out[i]=87;
        Matrix m={0,0,25,p,a,b,0}; AccessTarget t={matrix_request,&m};
        if(audit_access(&t,p,25,out+1)!=total) bad++;
        for(int i=0;i<25;i++) if(out[i+1]!=want[i]) bad++;
        if(out[0]!=87 || out[26]!=87) bad++;
        order+=m.bad || m.call!=25 || m.replay;
    }
    CHECK("classify controls and replay evidence",bad==0,"%d verdict or boundary errors",bad);
    CHECK("only approved calls in order",order==0,"%d callback sequence errors",order);
    CHECK("empty matrix",audit_access(NULL,NULL,0,NULL)==0,"empty input requires no target");
    return 0;
}
