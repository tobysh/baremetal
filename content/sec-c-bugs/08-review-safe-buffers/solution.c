#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int record_copy(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 if(len<0 || !dst)return 0;size_t n=(size_t)len;
 if(n>=cap || (n && !src))return 0;
 if(n)memcpy(dst,src,n);dst[n]=0;return 1;
}
int main(void){return 0;}
