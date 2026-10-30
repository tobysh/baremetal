#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct { char id[24]; int type; long cost; int mit; char kid[16][24]; int nk; } N;
static N nd[120]; static int nn;
static int find(const char *id){ for(int i=0;i<nn;i++) if(!strcmp(nd[i].id,id))return i; return -1; }
static long cost(int i,int honour){
  N *x=&nd[i];
  if(x->type==0)return (honour&&x->mit)?-1:x->cost;
  long best=-1,sum=0;
  for(int k=0;k<x->nk;k++){
    int c=find(x->kid[k]); long v=c<0?-1:cost(c,honour);
    if(x->type==1){ if(v<0)return -1; sum+=v; }
    else if(v>=0&&(best<0||v<best))best=v;
  }
  return x->type==1?sum:best;
}
static int first=1;
static void path(int i){
  N *x=&nd[i];
  if(x->type==0){ printf("%s%s",first?"":" ",x->id); first=0; return; }
  if(x->type==1){ for(int k=0;k<x->nk;k++) path(find(x->kid[k])); return; }
  long best=-1; int bi=-1;
  for(int k=0;k<x->nk;k++){ int c=find(x->kid[k]); long v=c<0?-1:cost(c,1); if(v>=0&&(best<0||v<best)){best=v;bi=c;} }
  if(bi>=0)path(bi);
}
int main(void){
  char line[1024],root[24]="";
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; char *t=strtok(line," "); if(!t)continue;
    if(!strcmp(t,"root")){ t=strtok(NULL," "); if(t)snprintf(root,24,"%s",t); continue; }
    if(nn>=120)continue; N *x=&nd[nn];
    memset(x,0,sizeof *x);
    if(!strcmp(t,"leaf")){ x->type=0; t=strtok(NULL," "); if(!t)continue; snprintf(x->id,24,"%s",t); t=strtok(NULL," "); x->cost=t?atol(t):0; t=strtok(NULL," "); x->mit=t&&!strcmp(t,"mitigated"); nn++; }
    else if(!strcmp(t,"and")||!strcmp(t,"or")){ x->type=!strcmp(t,"and")?1:2; t=strtok(NULL," "); if(!t)continue; snprintf(x->id,24,"%s",t); while((t=strtok(NULL," "))&&x->nk<16)snprintf(x->kid[x->nk++],24,"%s",t); nn++; }
  }
  int r=find(root); if(r<0)return 0;
  printf("unmitigated cost: %ld\n",cost(r,0));
  long m=cost(r,1);
  if(m<0){ puts("mitigated cost: infeasible"); puts("cheapest path: none"); }
  else { printf("mitigated cost: %ld\n",m); printf("cheapest path: "); path(r); puts(""); }
  return 0;
}
