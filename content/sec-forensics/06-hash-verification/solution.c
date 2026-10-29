#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
uint64_t fnv1a64(const uint8_t *p,size_t n){uint64_t h=0xcbf29ce484222325ULL;for(size_t i=0;i<n;i++)h=(h^p[i])*0x100000001b3ULL;return h;}
typedef struct {const char *name;const uint8_t *data;size_t n;} Item;
int audit(const char *manifest,const Item *items,int n,int *status){int bad=0;
 for(int i=0;i<n;i++){status[i]=2;const char *p=manifest;
  while(*p){unsigned long long h;char name[256];if(sscanf(p,"%llx %255s",&h,name)==2 && !strcmp(name,items[i].name)){status[i]=fnv1a64(items[i].data,items[i].n)==(uint64_t)h?0:1;break;}const char *e=strchr(p,'\n');if(!e)break;p=e+1;}
  if(status[i])bad++;
 }return bad;}
int main(void){return 0;}
