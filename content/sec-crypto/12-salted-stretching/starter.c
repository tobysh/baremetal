#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
uint64_t toy_kdf(const char *pw,const uint8_t *salt,size_t slen,unsigned iters){
 uint64_t h=14695981039346656037ULL;
 for(const char*p=pw;p&&*p;p++)h=(h^(uint8_t)*p)*1099511628211ULL;
 (void)salt;(void)slen;(void)iters;return h;
}
int main(void){return 0;}
