#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int clean_path(const char *in,char *out,size_t cap){
 if(!out||cap==0)return 0;
 out[0]=0;if(!in||in[0]!='/')return 0;
 for(const char*p=in;*p;p++){unsigned char c=(unsigned char)*p;if(c<0x20||c==0x7f||c=='\\')return 0;}
 char*tmp=malloc(strlen(in)+2);if(!tmp)return 0;
 size_t w=0;tmp[w++]='/';
 const char*p=in;int ok=1;
 while(*p&&ok){
  while(*p=='/')p++;
  if(!*p)break;
  const char*s=p;while(*p&&*p!='/')p++;
  size_t l=(size_t)(p-s);
  if(l==1&&s[0]=='.')continue;
  if(l==2&&s[0]=='.'&&s[1]=='.'){
   if(w==1){ok=0;break;}
   while(tmp[w-1]!='/')w--;
   if(w>1)w--;
   continue;}
  if(w>1)tmp[w++]='/';
  memcpy(tmp+w,s,l);w+=l;
 }
 if(ok&&w<cap){memcpy(out,tmp,w);out[w]=0;}else ok=0;
 free(tmp);return ok;
}
int main(void){return 0;}
