#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int bind_query(const char *tmpl,const char *const *args,int nargs,char *out,size_t cap){
  if(!out||cap==0||!tmpl)return -1;
  out[0]=0;
  int ph=0,inq=0;
  for(const char *p=tmpl;*p;p++){ if(*p=='\'')inq=!inq; else if(*p=='?'&&!inq)ph++; }
  if(inq||ph!=nargs)return -1;
  for(int i=0;i<nargs;i++){ if(!args||!args[i])return -1; for(const char *p=args[i];*p;p++) if((unsigned char)*p<0x20)return -1; }
  size_t o=0; int ai=0; inq=0;
#define PUT(ch) do{ if(o+1>=cap){out[0]=0;return -1;} out[o++]=(char)(ch);}while(0)
  for(const char *p=tmpl;*p;p++){
    if(*p=='\''){ inq=!inq; PUT(*p); }
    else if(*p=='?'&&!inq){ PUT('\''); for(const char *v=args[ai];*v;v++){ if(*v=='\'')PUT('\''); PUT(*v); } PUT('\''); ai++; }
    else PUT(*p);
  }
  out[o]=0; return (int)o;
}
int main(void){return 0;}
