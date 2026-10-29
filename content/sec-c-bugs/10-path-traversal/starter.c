#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int clean_path(const char *in,char *out,size_t cap){
 if(!in||!out||cap==0)return 0;
 if(strlen(in)>=cap)return 0;
 strcpy(out,in);return 1;
}
int main(void){return 0;}
