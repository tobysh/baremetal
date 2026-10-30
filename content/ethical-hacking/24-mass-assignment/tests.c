typedef struct { int step,phase,bad,win,fail,control; Account base; const char **fields,**values; } Profiles;
static int profile_update(void *v,const char *field,const char *value) {
    Profiles *p=v;
    if(p->phase) p->bad++;
    int i=p->step;
    const char *f=i?p->fields[(i-1)%4]:"display_name", *val=i?p->values[(i-1)%4]:"Lab Tester";
    if(strcmp(field,f)||strcmp(value,val)) p->bad++;
    p->phase=1;
    return i?403:(p->control==1?403:204);
}
static AccountReply profile_read(void *v) {
    Profiles *p=v; if(!p->phase) p->bad++; p->phase=0;
    int i=p->step++; AccountReply r={1,p->base};
    if(!i) {
        strcpy(r.account.display_name,p->control==2?"unchanged":"Lab Tester");
        if(p->control==3) r.account.role++;
        if(p->control==4) r.ok=0;
    } else {
        if(i-1==p->win) { if(i%2) r.account.role++; else r.account.credit--; }
        if(i-1==p->fail) r.ok=0;
    }
    return r;
}
int main(void) {
    const char *fields[]={"role","credit","is_staff","preferences"}, *values[]={"admin","999","true","compact"};
    int bad=0,seq=0;
    for(int mode=0;mode<5;mode++) for(int win=-1;win<4;win++) for(int fail=-1;fail<4;fail++) {
        Account base={"Original",7+win,80+fail}; Profiles p={0,0,0,win,fail,mode,base,fields,values};
        ProfileTarget t={profile_update,profile_read,&p}; int want=-1,calls=5;
        if(mode) {want=-2;calls=1;}
        else for(int i=0;i<4;i++) {
            if(i==fail) {want=-2;calls=i+2;break;}
            if(i==win) {want=i;calls=i+2;break;}
        }
        if(probe_fields(&t,base,fields,values,4)!=want) bad++;
        if(p.bad||p.phase||p.step!=calls) seq++;
    }
    CHECK("prove persisted changes or report inconclusive",bad==0,"%d result errors",bad);
    CHECK("one update and fresh read per approved probe",seq==0,"%d sequence errors",seq);
    Account base={"Original",1,50}; Profiles p={0,0,0,-1,-1,0,base,fields,values}; ProfileTarget t={profile_update,profile_read,&p};
    CHECK("empty list checks normal editing",probe_fields(&t,base,NULL,NULL,0)==-1 && p.step==1 && !p.bad,"control required for empty list");
    return 0;
}
