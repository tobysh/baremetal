#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct { char tok[40],user[40]; long t; int used; } K;
static K k[1100]; static int nk;
int main(void){
  long ttl; if(scanf("%ld",&ttl)!=1)return 0;
  long t; char act[16],a[64],b[64]; int n=0;
  while(scanf("%ld %15s %63s %63s",&t,act,a,b)==4){
    n++; const char *res="invalid"; char buf[80];
    if(!strcmp(act,"issue")){
      int dup=0; for(int i=0;i<nk;i++) if(!strcmp(k[i].tok,b))dup=1;
      if(dup)res="ignored-duplicate"; else if(nk<1100){ snprintf(k[nk].tok,40,"%s",b); snprintf(k[nk].user,40,"%s",a); k[nk].t=t; k[nk].used=0; nk++; res="ok"; }
    } else {
      int i=-1; for(int j=0;j<nk;j++) if(!strcmp(k[j].tok,a))i=j;
      if(i>=0){
        int cur=-1; for(int j=0;j<nk;j++) if(!strcmp(k[j].user,k[i].user))cur=j;
        if(cur!=i)res="superseded";
        else if(k[i].used)res="already-used";
        else if(t-k[i].t>=ttl)res="expired";
        else if(strlen(b)<8)res="weak-password";
        else { k[i].used=1; snprintf(buf,sizeof buf,"ok user=%s",k[i].user); res=buf; }
      }
    }
    printf("%d: %s\n",n,res);
  }
  return 0;
}
