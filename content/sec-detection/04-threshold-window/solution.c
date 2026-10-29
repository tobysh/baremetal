#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static long ts[2100]; static char key[2100][64];
int main(void){
  long W; int N; if(scanf("%ld %d",&W,&N)!=2)return 0;
  int n=0; char k[64]; long t; static char sk[70][64]; static long su[70]; int nk=0; int any=0;
  while(n<2100&&scanf("%ld %63s",&t,k)==2){
    ts[n]=t; strcpy(key[n],k);
    int cnt=0; for(int i=n;i>=0;i--) if(!strcmp(key[i],k)&&ts[i]>t-W)cnt++;
    int ki=-1; for(int i=0;i<nk;i++) if(!strcmp(sk[i],k))ki=i;
    if(ki<0&&nk<70){ki=nk++;strcpy(sk[ki],k);su[ki]=-1;}
    if(cnt>=N&&(ki<0||t>=su[ki])){printf("%ld %s count=%d\n",t,k,cnt);any=1;if(ki>=0)su[ki]=t+W;}
    n++;
  }
  if(!any)puts("no alerts");
  return 0;
}
