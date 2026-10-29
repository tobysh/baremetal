#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
const char *identify(const uint8_t *p,size_t n){
 static const struct {const char *name,*sig;size_t n;} s[]={
 {"PNG","\211PNG\r\n\032\n",8},{"JPEG","\377\330\377",3},{"GIF","GIF87a",6},{"GIF","GIF89a",6},
 {"PDF","%PDF-",5},{"ZIP","PK\003\004",4},{"ELF","\177ELF",4}};
 for(size_t i=0;i<sizeof s/sizeof s[0];i++)if(n>=s[i].n && !memcmp(p,s[i].sig,s[i].n))return s[i].name;
 return "unknown";
}
int main(void){return 0;}
