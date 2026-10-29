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
uint64_t fnv1a64(const uint8_t *p,size_t n){uint64_t h=0xcbf29ce484222325ULL;for(size_t i=0;i<n;i++)h=(h^p[i])*0x100000001b3ULL;return h;}
void report(const char *name,const uint8_t *p,size_t n,uint64_t expected,char *out,size_t cap){char preview[24];size_t k=0;
 if(!n){strcpy(preview,"-");}else{for(size_t i=0;i<n&&i<8;i++){if(i)preview[k++]=' ';snprintf(preview+k,sizeof preview-k,"%02x",p[i]);k+=2;}}
 const char *type=identify(p,n);if(!strcmp(type,"ELF")||!strcmp(type,"GIF"))type="unknown";uint64_t h=fnv1a64(p,n);
 snprintf(out,cap,"%s: %s, first bytes %s, hash %016llx, %s\n",name,type,preview,(unsigned long long)h,h==expected?"OK":"CHANGED");}
int main(void){return 0;}
