#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct { char d[80]; int q,u,txt; long s; } Dom;
static Dom dm[200]; static int nd;
static char subs[2100][2][160]; static int ns;
int main(void){
  char line[512],cl[64],qn[300],qt[16]; long ts;
  while(fgets(line,sizeof line,stdin)){
    if(sscanf(line,"%ld %63s %299s %15s",&ts,cl,qn,qt)!=4)continue;
    for(char *c=qn;*c;c++) if(*c>='A'&&*c<='Z')*c+=32;
    size_t n=strlen(qn); int dots=0; size_t cut=0;
    for(size_t i=n;i-->0;) if(qn[i]=='.'){ if(++dots==2){cut=i+1;break;} }
    const char *dom=qn+cut; char sub[300]=""; 
    if(dots==2){ size_t sl=cut?cut-1:0; memcpy(sub,qn,sl); sub[sl]=0; }
    int di=-1; for(int i=0;i<nd;i++) if(!strcmp(dm[i].d,dom))di=i;
    if(di<0){ if(nd>=200)continue; di=nd++; snprintf(dm[di].d,80,"%s",dom); }
    dm[di].q++;
    if(!strcmp(qt,"TXT")||!strcmp(qt,"NULL"))dm[di].txt++;
    if(*sub){
      int seen=0; for(int i=0;i<ns;i++) if(!strcmp(subs[i][0],dom)&&!strcmp(subs[i][1],sub))seen=1;
      if(!seen&&ns<2100){ snprintf(subs[ns][0],160,"%s",dom); snprintf(subs[ns][1],160,"%s",sub); ns++; dm[di].u++; dm[di].s+=(long)strlen(sub); }
    }
  }
  int idx[200],k=0;
  for(int i=0;i<nd;i++) if(dm[i].u>=8&&dm[i].s>=20L*dm[i].u)idx[k++]=i;
  for(int i=0;i<k;i++)for(int j=i+1;j<k;j++){ Dom *a=&dm[idx[i]],*b=&dm[idx[j]]; if(b->u>a->u||(b->u==a->u&&strcmp(b->d,a->d)<0)){int t=idx[i];idx[i]=idx[j];idx[j]=t;} }
  if(!k){puts("no tunnelling suspected");return 0;}
  for(int i=0;i<k;i++){Dom *d=&dm[idx[i]];printf("%s unique=%d avg=%ld queries=%d txt=%d\n",d->d,d->u,d->s/d->u,d->q,d->txt);}
  return 0;
}
