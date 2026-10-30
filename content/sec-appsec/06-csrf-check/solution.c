#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static const char *fld(const char *line,const char *key,char *out,size_t cap){
  size_t kl=strlen(key); const char *p=line;
  while(*p){ while(*p==' ')p++; if(!strncmp(p,key,kl)&&p[kl]=='='){ p+=kl+1; size_t n=0; while(p[n]&&p[n]!=' ')n++; if(n>=cap)n=cap-1; memcpy(out,p,n); out[n]=0; return out; } while(*p&&*p!=' ')p++; }
  out[0]=0; return NULL;
}
int main(void){
  char line[1024],site[128]="",sid[100][64],tok[100][64]; int ns=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; if(!*line)break;
    if(!strncmp(line,"SITE ",5))snprintf(site,128,"%s",line+5);
    else if(!strncmp(line,"SESSION ",8)&&ns<100&&sscanf(line+8,"%63s %63s",sid[ns],tok[ns])==2)ns++;
  }
  int n=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; n++;
    char m[16],path[200],s[64],o[128],t[64];
    fld(line,"method",m,sizeof m); fld(line,"path",path,sizeof path); fld(line,"sid",s,sizeof s); fld(line,"origin",o,sizeof o); fld(line,"token",t,sizeof t);
    printf("request %d: ",n);
    if(!strcmp(m,"GET")||!strcmp(m,"HEAD")||!strcmp(m,"OPTIONS")){
      puts(strstr(path,"/delete")||strstr(path,"/transfer")?"deny (state-change-over-get)":"allow"); continue;
    }
    int k=-1; if(strcmp(s,"-")) for(int i=0;i<ns;i++) if(!strcmp(sid[i],s))k=i;
    if(k<0){puts("deny (no-session)");continue;}
    if(strcmp(o,"-")&&strcmp(o,site)){puts("deny (cross-origin)");continue;}
    if(!strcmp(t,"-")||strcmp(t,tok[k])){puts("deny (bad-token)");continue;}
    puts("allow");
  }
  return 0;
}
