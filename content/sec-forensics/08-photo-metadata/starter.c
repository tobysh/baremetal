#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {char make[64],model[64];uint32_t width,height;char datetime[32];} Meta;
int read_meta(const uint8_t *p,size_t n,Meta *out){(void)p;(void)n;(void)out;return -1;}
long long to_unix(const char *s){(void)s;return -1;}
int main(void){return 0;}
