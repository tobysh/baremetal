#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct{char key[64],nonce[64],msgs[16][64];int nm;}G;
static G g[500];
static int cmp(const void*a,const void*b){const G*x=*(G*const*)a,*y=*(G*const*)b;int c=strcmp(x->key,y->key);return c?c:strcmp(x->nonce,y->nonce);}
int main(void){char line[1024];int n=0;
 while(fgets(line,sizeof line,stdin)){char k[64],nc[64],m[64];
  if(sscanf(line,"key=%63s nonce=%63s msg=%63s",k,nc,m)!=3)continue;
  int ok=1;for(char*p=nc;*p;p++){if(!((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f')||(*p>='A'&&*p<='F')))ok=0;else if(*p>='A'&&*p<='F')*p+=32;}
  if(!ok)continue;
  int j=0;while(j<n&&(strcmp(g[j].key,k)||strcmp(g[j].nonce,nc)))j++;
  if(j==n){if(n==500)continue;strcpy(g[n].key,k);strcpy(g[n].nonce,nc);n++;}
  int d=0;for(int i=0;i<g[j].nm;i++)if(!strcmp(g[j].msgs[i],m))d=1;
  if(!d&&g[j].nm<16)strcpy(g[j].msgs[g[j].nm++],m);}
 G*idx[500];int c=0;for(int i=0;i<n;i++)if(g[i].nm>=2)idx[c++]=&g[i];
 qsort(idx,c,sizeof idx[0],cmp);
 if(!c)puts("no nonce reuse");
 for(int i=0;i<c;i++){printf("%s nonce=%s msgs=",idx[i]->key,idx[i]->nonce);for(int j=0;j<idx[i]->nm;j++)printf("%s%s",j?",":"",idx[i]->msgs[j]);putchar('\n');}
 return 0;}
