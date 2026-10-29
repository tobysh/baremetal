#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
uint8_t tiny_hash(const uint8_t *p,size_t n){uint32_t h=0x811c9dc5u;for(size_t i=0;i<n;i++)h=(h^p[i])*0x01000193u;return (uint8_t)h;}
int first_collision(uint16_t *a,uint16_t *b){if(!a||!b)return 0;int seen[256];for(int i=0;i<256;i++)seen[i]=-1;
 for(unsigned v=0;v<65536;v++){uint8_t p[2]={(uint8_t)v,(uint8_t)(v>>8)};unsigned h=tiny_hash(p,2);if(seen[h]>=0){*a=(uint16_t)seen[h];*b=(uint16_t)v;return 1;}seen[h]=(int)v;}return 0;}
int main(void){return 0;}
