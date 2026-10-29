#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
typedef struct {uint16_t src,dst;size_t payload;} UDP;
int parse_udp(const uint8_t *p,size_t n,UDP *out){
 if(!p||!out||n<8)return -1;size_t len=be16(p+4);if(len<8||len>n)return -1;*out=(UDP){be16(p),be16(p+2),len-8};return 0;
}
int main(void){return 0;}
