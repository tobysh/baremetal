typedef struct { int calls,bad,n; const char **paths; FileReply *r; } Files;
static FileReply file_fetch(void *v,const char *path) {
    Files *f=v; int i=f->calls++;
    if(i>=f->n) { f->bad++; return (FileReply){500,NULL}; }
    if(strcmp(path,f->paths[i])) f->bad++;
    return f->r[i];
}
int main(void) {
    const char *c[]={"../lab-marker","%2e%2e/lab-marker","..%2flab-marker","%252e%252e%252flab-marker","folder/../../lab-marker"};
    int bad=0,seq=0;
    for(int trial=0;trial<30;trial++) {
        char marker[40],near[80]; snprintf(marker,sizeof marker,"LAB-MARKER-%d",trial);
        snprintf(near,sizeof near,"error: %s",marker);
        const char *paths[6]={"readme.txt",c[0],c[1],c[2],c[3],c[4]};
        FileReply r[6]={{200,"control"},{200,near},{404,marker},{200,NULL},{200,"other"},{403,marker}};
        int win=trial%6-1; if(win>=0) r[win+1]=(FileReply){200,marker};
        int broken=trial%5==0;
        if(broken) r[0]=(FileReply){trial%2?403:200,trial%2?"control":NULL};
        Files f={0,0,6,paths,r}; FileTarget t={file_fetch,&f};
        int got=find_traversal(&t,"readme.txt","control",marker,c,5);
        int want=broken?-2:win;
        if(got!=want) bad++;
        if(f.bad || f.calls!=(broken?1:win>=0?win+2:6)) seq++;
    }
    CHECK("exact marker evidence and failed controls",bad==0,"%d classifications wrong",bad);
    CHECK("preserve probe spelling and stop on proof",seq==0,"%d request sequence errors",seq);
    const char *paths[]={"readme.txt"}; FileReply r[]={{200,"control"}};
    Files f={0,0,1,paths,r}; FileTarget t={file_fetch,&f};
    CHECK("empty candidate list still checks control",find_traversal(&t,"readme.txt","control","marker",NULL,0)==-1 && f.calls==1 && !f.bad,"fetch the control once");
    return 0;
}
