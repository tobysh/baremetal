#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
size_t literal_message(char *dst,size_t cap,const char *text){
 if(!text)text="";
 int n=snprintf(dst,cap,"%s",text);return n<0?0:(size_t)n;
}
int main(void){return 0;}
