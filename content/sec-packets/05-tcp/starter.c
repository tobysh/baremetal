#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint16_t src,dst;uint32_t seq,ack;uint8_t flags;size_t header,payload;} TCP;
int parse_tcp(const uint8_t *p,size_t n,TCP *out){(void)p;(void)n;(void)out;return -1;}
int main(void){return 0;}
