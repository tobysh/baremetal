#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int copy_signed(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 if(len<0)return -1;
 size_t n=(size_t)len;
 if(n>cap || (n && (!dst || !src)))return -1;
 if(n)memcpy(dst,src,n);return len;
}
int main(void){return 0;}
