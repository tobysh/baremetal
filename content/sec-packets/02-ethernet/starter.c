#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint8_t dst[6],src[6];uint16_t type;size_t payload;} Ethernet;
int parse_ethernet(const uint8_t *p,size_t n,Ethernet *out){(void)p;(void)n;(void)out;return -1;}
int main(void){return 0;}
