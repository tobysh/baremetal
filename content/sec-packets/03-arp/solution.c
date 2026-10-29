#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
typedef struct {uint16_t opcode;uint32_t sender,target;} ARP;
int parse_arp(const uint8_t *p,size_t n,ARP *out){if(!p||!out||n<28 || be16(p)!=1 || be16(p+2)!=0x0800 || p[4]!=6 || p[5]!=4)return -1;
 uint16_t op=be16(p+6);if(op!=1&&op!=2)return -1;*out=(ARP){op,be32(p+14),be32(p+24)};return 0;}
int main(void){return 0;}
