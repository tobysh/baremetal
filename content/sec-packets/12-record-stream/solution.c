#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int tls_walk(const uint8_t *b,size_t n,size_t *records,size_t *consumed){
 size_t pos=0,rc=0;int ret=0;
 if(n&&!b){if(records)*records=0;if(consumed)*consumed=0;return -1;}
 for(;;){
  size_t rem=n-pos;
  if(rem==0){ret=0;break;}
  if(rem<5){ret=1;break;}
  size_t l=((size_t)b[pos+3]<<8)|b[pos+4];
  if(b[pos]<20||b[pos]>23||b[pos+1]!=3||b[pos+2]<1||b[pos+2]>4||l<1||l>16384){ret=-1;break;}
  if(l>rem-5){ret=1;break;}
  pos+=5+l;rc++;
 }
 if(records)*records=rc;if(consumed)*consumed=pos;return ret;
}
int main(void){return 0;}
