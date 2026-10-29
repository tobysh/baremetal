#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int build_label(char *out,size_t cap,const char *user,const char *host){
 if(!out)return 0;
 snprintf(out,cap,"%s@%s",user,host);
 return 1;
}
int main(void){return 0;}
