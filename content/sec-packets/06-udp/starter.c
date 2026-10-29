#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint16_t src,dst;size_t payload;} UDP;
int parse_udp(const uint8_t *p,size_t n,UDP *out){(void)p;(void)n;(void)out;return -1;}
int main(void){return 0;}
