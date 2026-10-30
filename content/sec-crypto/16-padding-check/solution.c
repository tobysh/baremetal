#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int pkcs7_unpad(const uint8_t *blk,size_t n,size_t bs,size_t *datalen,size_t *work){
 if(datalen)*datalen=0;if(work)*work=0;
 if(!blk||bs==0||bs>255||n==0||n%bs)return 0;
 uint8_t p=blk[n-1];unsigned bad=(p==0)|(p>bs);size_t w=0;
 for(size_t i=0;i<bs;i++){unsigned in=i<p;bad|=in&(blk[n-1-i]!=p);w++;}
 if(work)*work=w;
 if(bad)return 0;
 if(datalen)*datalen=n-p;return 1;
}
int main(void){return 0;}
