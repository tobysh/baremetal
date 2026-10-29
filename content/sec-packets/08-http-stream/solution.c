#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint32_t seq;const uint8_t *data;size_t n;} Segment;
int http_path(const Segment *s,size_t count,uint32_t base,char *out,size_t cap){if(!out||!cap||(count&&!s))return -1;uint8_t buf[1025]={0},seen[1024]={0};size_t end=0;
 for(size_t i=0;i<count;i++){if(!s[i].n)continue;if(!s[i].data||s[i].seq<base)return -1;uint32_t delta=s[i].seq-base;if(delta>1024||s[i].n>1024-delta)return -1;size_t off=delta;
  for(size_t j=0;j<s[i].n;j++){size_t k=off+j;if(seen[k]&&buf[k]!=s[i].data[j])return -1;buf[k]=s[i].data[j];seen[k]=1;}if(off+s[i].n>end)end=off+s[i].n;
 }for(size_t i=0;i<end;i++){if(!seen[i]||((buf[i]<32||buf[i]>126)&&buf[i]!='\r'&&buf[i]!='\n'))return -1;}buf[end]=0;
 char *text=(char*)buf;if(end<18||strncmp(text,"GET ",4)||!strstr(text,"\r\n\r\n"))return -1;char *space=strchr(text+4,' ');if(!space||space==text+4||strncmp(space," HTTP/1.1\r\n",11))return -1;
 size_t n=(size_t)(space-(text+4));for(size_t i=0;i<n;i++)if((unsigned char)text[4+i]<33)return -1;if(n>=cap)return -1;memcpy(out,text+4,n);out[n]=0;return (int)n;}
int main(void){return 0;}
