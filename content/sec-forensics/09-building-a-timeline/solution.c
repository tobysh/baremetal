#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {long long t;char src[64],msg[512];} Event;
int main(void){Event ev[1000];size_t n=0;char line[512];
 while(n<1000 && fgets(line,sizeof line,stdin)){int at=0;if(sscanf(line,"%lld %63s %n",&ev[n].t,ev[n].src,&at)!=2)continue;strcpy(ev[n].msg,line+at);ev[n].msg[strcspn(ev[n].msg,"\r\n")]=0;n++;}
 for(size_t i=1;i<n;i++){Event v=ev[i];size_t j=i;while(j && ev[j-1].t>v.t){ev[j]=ev[j-1];j--;}ev[j]=v;}
 for(size_t i=0;i<n;i++){if(i && ev[i].t-ev[i-1].t>=3600){long long g=ev[i].t-ev[i-1].t;printf("-- quiet for %lldh %02lldm --\n",g/3600,g%3600/60);}time_t t=(time_t)ev[i].t;struct tm tm=*gmtime(&t);char out[32];strftime(out,sizeof out,"%Y-%m-%dT%H:%M:%SZ",&tm);printf("%s [%s] %s\n",out,ev[i].src,ev[i].msg);}
 long long span=n?ev[n-1].t-ev[0].t:0;printf("%zu events over %lldh %02lldm\n",n,span/3600,span%3600/60);return 0;}
