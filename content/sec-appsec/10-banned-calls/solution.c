#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

static const char *B[]={"gets","strcpy","strcat","sprintf","vsprintf","system","popen","tmpnam","mktemp"};
static const char *R[]={"fgets","snprintf","snprintf","snprintf","vsnprintf","execv","execv","mkstemp","mkstemp"};
static char src[40000];
int main(void){
  size_t n=fread(src,1,sizeof src-1,stdin); src[n]=0;
  int line=1,col=1,found=0; size_t i=0; enum{CODE,LC,BC,STR,CH} st=CODE;
  while(i<n){
    char c=src[i];
    if(st==CODE){
      if(c=='/'&&src[i+1]=='/'){st=LC;i+=2;col+=2;continue;}
      if(c=='/'&&src[i+1]=='*'){st=BC;i+=2;col+=2;continue;}
      if(c=='"'){st=STR;i++;col++;continue;}
      if(c=='\''){st=CH;i++;col++;continue;}
      if(isalpha((unsigned char)c)||c=='_'||isdigit((unsigned char)c)){
        size_t j=i; while(j<n&&(isalnum((unsigned char)src[j])||src[j]=='_'))j++;
        size_t l=j-i; int isid=!isdigit((unsigned char)c);
        if(isid) for(int k=0;k<9;k++) if(strlen(B[k])==l&&!strncmp(src+i,B[k],l)){
          size_t p=j; while(p<n&&(src[p]==' '||src[p]=='\t'))p++;
          int call=p<n&&src[p]=='(';
          size_t q=i; while(q>0&&(src[q-1]==' '||src[q-1]=='\t'))q--;
          int member=q>0&&(src[q-1]=='.'||(q>1&&src[q-1]=='>'&&src[q-2]=='-'));
          if(call&&!member){ printf("%d:%d %s -> %s\n",line,col,B[k],R[k]); found++; }
        }
        col+=(int)l; i=j; continue;
      }
    } else if(st==LC){ if(c=='\n')st=CODE; }
    else if(st==BC){ if(c=='*'&&src[i+1]=='/'){st=CODE;i+=2;col+=2;continue;} }
    else if(st==STR||st==CH){
      if(c=='\\'&&i+1<n){ if(src[i+1]=='\n'){line++;col=0;} i+=2;col+=2; continue; }
      if((st==STR&&c=='"')||(st==CH&&c=='\''))st=CODE;
    }
    if(c=='\n'){line++;col=1;} else col++;
    i++;
  }
  if(!found)puts("no banned calls"); else printf("%d banned call%s\n",found,found==1?"":"s");
  return 0;
}
