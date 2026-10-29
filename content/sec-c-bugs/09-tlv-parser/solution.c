#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int tlv_find(const uint8_t *buf,size_t len,uint8_t type,size_t *off,size_t *vlen){
 if(off)*off=0;if(vlen)*vlen=0;
 if(len&&!buf)return -1;
 size_t pos=0;
 while(pos<len){
  if(len-pos<3)return -1;
  uint8_t t=buf[pos];size_t l=((size_t)buf[pos+1]<<8)|buf[pos+2];
  pos+=3;
  if(l>len-pos)return -1;
  if(t==type){if(off)*off=pos;if(vlen)*vlen=l;return 1;}
  pos+=l;
 }
 return 0;
}
int main(void){return 0;}
