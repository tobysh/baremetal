#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int equal_bytes(const uint8_t *a,const uint8_t *b,size_t n,size_t *visited){
 if(visited)*visited=0;if(n && (!a||!b))return 0;
 unsigned diff=0;for(size_t i=0;i<n;i++){diff|=a[i]^b[i];if(visited)(*visited)++;}return diff==0;
}
int main(void){return 0;}
