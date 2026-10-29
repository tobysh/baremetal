#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct { long t; char host[32],user[32]; int idx; } Ev;
typedef struct { char user[32]; long first,last; int n; char hosts[8][32]; int nh; } Burst;
static Ev ev[1100]; static Burst bs[1100]; static int nb;
static int cmpb(const void *a,const void *b){ const Burst *x=a,*y=b; if(x->first!=y->first)return x->first<y->first?-1:1; return strcmp(x->user,y->user); }
static int cmpe(const void *a,const void *b){ const Ev *x=a,*y=b; if(x->t!=y->t)return x->t<y->t?-1:1; return x->idx-y->idx; }
int main(void){
  long G; char line[512]; int n=0;
  if(!fgets(line,sizeof line,stdin)||sscanf(line,"%ld",&G)!=1)return 0;
  while(n<1100&&fgets(line,sizeof line,stdin)) if(sscanf(line,"%ld %31s %31s",&ev[n].t,ev[n].host,ev[n].user)==3){ev[n].idx=n;n++;}
  qsort(ev,n,sizeof(Ev),cmpe);
  static int cur[100]; static char names[100][32]; int nu=0;
  for(int i=0;i<n;i++){
    int u=-1; for(int k=0;k<nu;k++) if(!strcmp(names[k],ev[i].user))u=k;
    if(u<0){u=nu++;strcpy(names[u],ev[i].user);cur[u]=-1;}
    if(cur[u]<0||ev[i].t-bs[cur[u]].last>G){ cur[u]=nb++; Burst *b=&bs[cur[u]]; strcpy(b->user,ev[i].user); b->first=ev[i].t; b->n=0; b->nh=0; }
    Burst *b=&bs[cur[u]]; b->last=ev[i].t; b->n++;
    int h=0; for(int k=0;k<b->nh;k++) if(!strcmp(b->hosts[k],ev[i].host))h=1;
    if(!h&&b->nh<8)strcpy(b->hosts[b->nh++],ev[i].host);
  }
  qsort(bs,nb,sizeof(Burst),cmpb);
  for(int i=0;i<nb;i++){
    printf("%s %ld-%ld n=%d hosts=",bs[i].user,bs[i].first,bs[i].last,bs[i].n);
    for(int k=0;k<bs[i].nh;k++)printf("%s%s",k?",":"",bs[i].hosts[k]);
    puts(bs[i].nh>1?" MULTI-HOST":"");
  }
  return 0;
}
