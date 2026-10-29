#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
void release(void **owner){
 if(!owner)return;free(*owner); /* TODO: reset the caller's owner. */
}
int main(void){return 0;}
