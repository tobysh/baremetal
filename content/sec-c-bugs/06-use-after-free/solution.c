#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
char *owned_label(const char *src){
 if(!src)return NULL;size_t n=strlen(src);if(n==SIZE_MAX)return NULL;
 char *p=malloc(n+1);if(!p)return NULL;memcpy(p,src,n+1);return p;
}
int main(void){return 0;}
