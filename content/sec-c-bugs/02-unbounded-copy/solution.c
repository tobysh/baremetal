#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int copy_bytes(uint8_t *dst,size_t cap,const uint8_t *src,size_t n) {
    if(n>cap || (n && (!dst || !src))) return 0;
    if(n) memcpy(dst,src,n);
    return 1;
}
int main(void){return 0;}
