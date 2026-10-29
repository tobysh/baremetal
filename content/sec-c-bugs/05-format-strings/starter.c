#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
size_t literal_message(char *dst,size_t cap,const char *text){
 /* TODO: preserve all of text rather than dropping it. Never use text as a format. */
 (void)text;if(cap)dst[0]=0;return 0;
}
int main(void){return 0;}
