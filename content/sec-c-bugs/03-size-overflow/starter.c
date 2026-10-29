#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int allocation_size(size_t count,size_t width,size_t *out){
 if(!out)return 0; *out=count*width+1; return 1;
}
int main(void){return 0;}
