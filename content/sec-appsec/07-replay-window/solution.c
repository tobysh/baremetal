#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static char nn[1100][40]; static long nt[1100]; static int live;
int main(void){
  long W; int cap; if(scanf("%ld %d",&W,&cap)!=2)return 0;
  long mt,rt; char nc[64]; int n=0;
  while(scanf("%ld %ld %63s",&mt,&rt,nc)==3){
    n++; const char *res;
    if(mt<rt-W)res="stale"; else if(mt>rt+W)res="future";
    else {
      int k=0; for(int i=0;i<live;i++) if(nt[i]>=rt-W){ if(k!=i){strcpy(nn[k],nn[i]);nt[k]=nt[i];} k++; } live=k;
      int dup=0; for(int i=0;i<live;i++) if(!strcmp(nn[i],nc))dup=1;
      if(dup)res="replay"; else if(live>=cap)res="cache-full"; else { snprintf(nn[live],40,"%s",nc); nt[live++]=mt; res="accept"; }
    }
    printf("%d: %s\n",n,res);
  }
  return 0;
}
