#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {const char *type;size_t offset,length;} Carved;
size_t carve(const uint8_t *p,size_t n,Carved *out,size_t max){(void)p;(void)n;(void)out;(void)max;return 0;}
int main(void){return 0;}
