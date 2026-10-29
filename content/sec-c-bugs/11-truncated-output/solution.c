#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int build_label(char *out,size_t cap,const char *user,const char *host){
 if(!out||cap==0)return 0;
 if(!user||!host||!*user||!*host){out[0]=0;return 0;}
 int n=snprintf(out,cap,"%s@%s",user,host);
 if(n<0||(size_t)n>=cap){out[0]=0;return 0;}
 return 1;
}
int main(void){return 0;}
