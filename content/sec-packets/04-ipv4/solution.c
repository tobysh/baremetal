#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
typedef struct {uint32_t src,dst;uint16_t total;uint8_t protocol;size_t header;int fragmented;} IPv4;
int parse_ipv4(const uint8_t *p,size_t n,IPv4 *out){
 if(!p||!out||n<20 || p[0]>>4!=4)return -1;size_t h=(p[0]&15)*4;uint16_t total=be16(p+2);
 if(h<20 || h>n || total<h || total>n)return -1;
 *out=(IPv4){be32(p+12),be32(p+16),total,p[9],h,(be16(p+6)&0x3fff)!=0};return 0;
}
int main(void){return 0;}
