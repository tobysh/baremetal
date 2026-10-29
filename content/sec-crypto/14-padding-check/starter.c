#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int pkcs7_unpad(const uint8_t *blk,size_t n,size_t bs,size_t *datalen,size_t *work){
 if(datalen)*datalen=0;if(work)*work=0;
 if(!blk||bs==0||n==0)return 0;
 uint8_t p=blk[n-1];if(p==0||p>bs)return 0;
 if(datalen)*datalen=n-p;return 1;
}
int main(void){return 0;}
