#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint64_t fnv(uint64_t h,const uint8_t*p,size_t n){for(size_t i=0;i<n;i++)h=(h^p[i])*1099511628211ULL;return h;}
uint64_t toy_kdf(const char *pw,const uint8_t *salt,size_t slen,unsigned iters){
 const uint64_t basis=14695981039346656037ULL;
 if(!salt)slen=0;if(!pw)pw="";
 uint64_t h=fnv(basis,salt,slen);h=fnv(h,(const uint8_t*)pw,strlen(pw));
 if(!iters)iters=1;
 for(unsigned i=0;i<iters;i++){uint8_t b[8];for(int j=0;j<8;j++)b[j]=(uint8_t)(h>>(8*j));h=fnv(basis,b,8);h=fnv(h,salt,slen);}
 return h;
}
int main(void){return 0;}
