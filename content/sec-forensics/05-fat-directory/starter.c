#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {char name[13];int deleted,is_dir;uint32_t size,cluster;} DirEntry;
int read_dir(const uint8_t *p,size_t n,DirEntry *out,size_t max){(void)p;(void)n;(void)out;(void)max;return 0;}
int main(void){return 0;}
