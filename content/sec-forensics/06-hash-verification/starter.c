#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {const char *name;const uint8_t *data;size_t n;} Item;
uint64_t fnv1a64(const uint8_t *p,size_t n){(void)p;(void)n;return 0;}
int audit(const char *manifest,const Item *items,int n,int *status){(void)manifest;(void)items;for(int i=0;i<n;i++)status[i]=2;return n;}
int main(void){return 0;}
