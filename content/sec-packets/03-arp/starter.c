#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint16_t opcode;uint32_t sender,target;} ARP;
int parse_arp(const uint8_t *p,size_t n,ARP *out){(void)p;(void)n;(void)out;return -1;}
int main(void){return 0;}
