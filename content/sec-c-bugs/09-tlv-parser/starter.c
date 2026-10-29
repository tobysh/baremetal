#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int tlv_find(const uint8_t *buf,size_t len,uint8_t type,size_t *off,size_t *vlen){
 size_t pos=0;if(off)*off=0;if(vlen)*vlen=0;
 while(pos<len){uint8_t t=buf[pos];size_t l=((size_t)buf[pos+1]<<8)|buf[pos+2];pos+=3;
  if(t==type){if(off)*off=pos;if(vlen)*vlen=l;return 1;}pos+=l;}
 return 0;
}
int main(void){return 0;}
