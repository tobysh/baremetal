#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct { char sid[40]; int live; long created,last; } S;
static S s[1100];
int main(void){
  long idle,ab; if(scanf("%ld %ld",&idle,&ab)!=2)return 0;
  int ns=0,n=0; long t; char act[16],sid[64];
  while(scanf("%ld %15s %63s",&t,act,sid)==3){
    n++; int k=-1; for(int i=0;i<ns;i++) if(!strcmp(s[i].sid,sid))k=i;
    const char *res="unknown";
    if(!strcmp(act,"login")){
      if(k>=0)res="rejected-reused-id"; else if(ns<1100){ snprintf(s[ns].sid,40,"%s",sid); s[ns].live=1; s[ns].created=s[ns].last=t; ns++; res="ok"; }
    } else if(!strcmp(act,"req")){
      if(k>=0&&s[k].live){
        if(t-s[k].created>ab){s[k].live=0;res="expired-absolute";}
        else if(t-s[k].last>idle){s[k].live=0;res="expired-idle";}
        else {s[k].last=t;res="ok";}
      }
    } else if(!strcmp(act,"logout")){
      if(k>=0&&s[k].live){s[k].live=0;res="ok";}
    }
    printf("%d: %s\n",n,res);
  }
  return 0;
}
