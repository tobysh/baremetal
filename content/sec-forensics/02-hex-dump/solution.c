#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
size_t hexdump(const uint8_t *p,size_t n,char *out,size_t cap){size_t total=0;
 for(size_t off=0;off<n;off+=16){char row[100],hex[40],asc[17];size_t m=n-off<16?n-off:16,h=0;
  for(size_t i=0;i<m;i++){if(i && i%2==0)hex[h++]=' ';snprintf(hex+h,sizeof hex-h,"%02x",p[off+i]);h+=2;asc[i]=p[off+i]>=32&&p[off+i]<127?(char)p[off+i]:'.';}
  while(h<39)hex[h++]=' ';hex[h]=0;asc[m]=0;int k=snprintf(row,sizeof row,"%08zx: %s  %s\n",off,hex,asc);
  for(int i=0;i<k;i++){if(cap && total<cap-1)out[total]=row[i];total++;}
 }if(cap)out[total<cap?total:cap-1]=0;return total;}
int main(void){return 0;}
