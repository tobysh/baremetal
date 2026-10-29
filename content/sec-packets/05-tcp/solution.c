#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
typedef struct {uint16_t src,dst;uint32_t seq,ack;uint8_t flags;size_t header,payload;} TCP;
int parse_tcp(const uint8_t *p,size_t n,TCP *out){
 if(!p||!out||n<20)return -1;size_t h=(p[12]>>4)*4;if(h<20||h>n)return -1;
 *out=(TCP){be16(p),be16(p+2),be32(p+4),be32(p+8),p[13],h,n-h};return 0;
}
int main(void){return 0;}
