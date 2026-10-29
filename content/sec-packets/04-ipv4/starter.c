#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint32_t src,dst;uint16_t total;uint8_t protocol;size_t header;int fragmented;} IPv4;
int parse_ipv4(const uint8_t *p,size_t n,IPv4 *out){(void)p;(void)n;(void)out;return -1;}
int main(void){return 0;}
