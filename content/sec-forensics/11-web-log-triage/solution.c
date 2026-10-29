#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct{char ip[64];int hits;}S;static S s[500];
static int cmp(const void*a,const void*b){const S*x=*(S*const*)a,*y=*(S*const*)b;if(x->hits!=y->hits)return y->hits-x->hits;return strcmp(x->ip,y->ip);}
int main(void){char line[2048];int cls[6]={0},mal=0,n=0;
 while(fgets(line,sizeof line,stdin)){
  char ip[64];if(sscanf(line,"%63s",ip)!=1){mal++;continue;}
  char*q1=strchr(line,'"');char*q2=q1?strchr(q1+1,'"'):NULL;if(!q2){mal++;continue;}
  *q2=0;char me[16],pa[512],ve[16],ex[2];
  if(sscanf(q1+1,"%15s %511s %15s %1s",me,pa,ve,ex)!=3){mal++;continue;}
  int st;char by[32],ex2[2];
  if(sscanf(q2+1," %d %31s %1s",&st,by,ex2)!=2||st<100||st>599){mal++;continue;}
  int okb=strcmp(by,"-")==0;if(!okb){okb=by[0]!=0;for(char*p=by;*p;p++)if(*p<'0'||*p>'9')okb=0;}
  if(!okb){mal++;continue;}
  cls[st/100]++;
  for(char*p=pa;*p;p++)if(*p>='A'&&*p<='Z')*p+=32;
  if(strstr(pa,"../")||strstr(pa,"%2e%2e")||strstr(pa,"/etc/passwd")){
   int j=0;while(j<n&&strcmp(s[j].ip,ip))j++;if(j==n){if(n==500)continue;strcpy(s[n++].ip,ip);}s[j].hits++;}
 }
 printf("2xx=%d 3xx=%d 4xx=%d 5xx=%d malformed=%d\n",cls[2],cls[3],cls[4],cls[5],mal);
 S*idx[500];int c=0;for(int i=0;i<n;i++)if(s[i].hits>=2)idx[c++]=&s[i];
 qsort(idx,c,sizeof idx[0],cmp);
 for(int i=0;i<c;i++)printf("SUSPECT %s hits=%d\n",idx[i]->ip,idx[i]->hits);
 return 0;}
