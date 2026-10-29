#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int repeated_block(const uint8_t *p,size_t n,size_t b){
 if(!p || !b)return 0;size_t blocks=n/b;
 for(size_t i=0;i<blocks;i++)for(size_t j=i+1;j<blocks;j++)if(!memcmp(p+i*b,p+j*b,b))return 1;
 return 0;
}
int main(void){return 0;}
