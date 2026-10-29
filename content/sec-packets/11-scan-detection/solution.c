#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct{char ip[64];int n,o,c,f;char sv[256][64];int pt[256];}C;
static C cl[256];
int main(void){char line[1024];int nc=0;
 while(fgets(line,sizeof line,stdin)){char a[64],b[64],c[64],oc[32],ex[2];long t;int port;
  if(sscanf(line,"%ld %63s %63s %d %31s %1s",&t,b,c,&port,oc,ex)!=5)continue;(void)a;
  if(port<1||port>65535)continue;
  int k;if(!strcmp(oc,"open"))k=0;else if(!strcmp(oc,"closed"))k=1;else if(!strcmp(oc,"filtered"))k=2;else continue;
  int j=0;while(j<nc&&strcmp(cl[j].ip,b))j++;if(j==nc){if(nc==256)continue;strcpy(cl[nc++].ip,b);}
  C*x=&cl[j];int d=0;for(int i=0;i<x->n;i++)if(x->pt[i]==port&&!strcmp(x->sv[i],c))d=1;
  if(d||x->n==256)continue;strcpy(x->sv[x->n],c);x->pt[x->n++]=port;if(k==0)x->o++;else if(k==1)x->c++;else x->f++;}
 int ord[256],m=0;for(int i=0;i<nc;i++)if(cl[i].n>=5&&cl[i].c+cl[i].f>cl[i].o)ord[m++]=i;
 for(int i=1;i<m;i++){int v=ord[i],j=i;while(j&&(cl[ord[j-1]].n<cl[v].n||(cl[ord[j-1]].n==cl[v].n&&strcmp(cl[ord[j-1]].ip,cl[v].ip)>0))){ord[j]=ord[j-1];j--;}ord[j]=v;}
 if(!m)puts("no scans detected");
 for(int i=0;i<m;i++){C*x=&cl[ord[i]];printf("%s targets=%d open=%d closed=%d filtered=%d\n",x->ip,x->n,x->o,x->c,x->f);}
 return 0;}
