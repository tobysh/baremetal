#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint32_t seq;const uint8_t *data;size_t n;} Segment;
int http_path(const Segment *s,size_t count,uint32_t base,char *out,size_t cap){(void)s;(void)count;(void)base;(void)out;(void)cap;return -1;}
int main(void){return 0;}
