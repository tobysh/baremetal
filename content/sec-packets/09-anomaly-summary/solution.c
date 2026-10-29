#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint32_t src;uint16_t port;uint8_t flags;} Flow;
size_t scan_sources(const Flow *f,size_t n,size_t threshold,uint32_t *out,size_t max){if(!threshold||!max||!n)return 0;uint32_t candidates[128];size_t count=0;
 for(size_t i=0;i<n;i++){int prior=0;for(size_t j=0;j<i;j++)if(f[j].src==f[i].src)prior=1;if(prior)continue;size_t ports=0;
  for(size_t j=0;j<n;j++){if(f[j].src!=f[i].src || (f[j].flags&0x12)!=2)continue;int duplicate=0;for(size_t k=0;k<j;k++)if(f[k].src==f[j].src && f[k].port==f[j].port && (f[k].flags&0x12)==2)duplicate=1;if(!duplicate)ports++;}
  if(ports>=threshold)candidates[count++]=f[i].src;
 }for(size_t i=1;i<count;i++){uint32_t v=candidates[i];size_t j=i;while(j&&candidates[j-1]>v){candidates[j]=candidates[j-1];j--;}candidates[j]=v;}
 size_t stored=count<max?count:max;for(size_t i=0;i<stored;i++)out[i]=candidates[i];return stored;}
int main(void){return 0;}
