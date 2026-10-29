#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
size_t log_escape(char *out,size_t cap,const char *in){
 if(!in)in="";
 if(out&&cap){snprintf(out,cap,"%s",in);}
 return strlen(in);
}
int main(void){return 0;}
