#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int copy_signed(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 /* TODO: distinguish errors from a valid zero-byte request. */
 (void)dst;(void)cap;(void)src;return len;
}
int main(void){return 0;}
