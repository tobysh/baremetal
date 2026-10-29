#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
typedef struct {uint16_t id,type,qclass;char name[254];} Question;
int dns_question(const uint8_t *p,size_t n,Question *out){if(!p||!out||n<12||be16(p+4)!=1)return -1;Question q={0};q.id=be16(p);size_t off=12,k=0;
 while(1){if(off>=n)return -1;size_t len=p[off++];if(!len)break;if(len>63||len>n-off)return -1;if(k){if(k>=253)return -1;q.name[k++]='.';}if(len>253-k)return -1;
  for(size_t i=0;i<len;i++){unsigned c=p[off+i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'))return -1;q.name[k++]=(char)c;}off+=len;
 }if(!k)q.name[k++]='.';q.name[k]=0;if(n-off<4)return -1;q.type=be16(p+off);q.qclass=be16(p+off+2);*out=q;return 0;}
int main(void){return 0;}
