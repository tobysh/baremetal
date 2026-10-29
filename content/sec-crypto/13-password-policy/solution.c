#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int main(void){char l[512];int n=0;
 while(fgets(l,sizeof l,stdin)){n++;size_t len=strlen(l);while(len&&(l[len-1]=='\n'||l[len-1]=='\r'))l[--len]=0;
  int lo=0,up=0,di=0,ot=0,rep=0;char low[512];
  for(size_t i=0;i<len;i++){unsigned char c=(unsigned char)l[i];
   if(c>='a'&&c<='z')lo=1;else if(c>='A'&&c<='Z')up=1;else if(c>='0'&&c<='9')di=1;else ot=1;
   low[i]=(c>='A'&&c<='Z')?(char)(c+32):(char)c;
   if(i>=2&&l[i]==l[i-1]&&l[i]==l[i-2])rep=1;}
  low[len]=0;
  int cl=lo+up+di+ot,com=strstr(low,"password")||strstr(low,"letmein")||strstr(low,"qwerty")||strstr(low,"123456");
  char out[64]="";int k=0;
  #define ADD(c,s) if(c){strcat(out,k++?",":"");strcat(out,s);}
  ADD(len<10,"short") ADD(len>64,"long") ADD(cl<3,"classes") ADD(com,"common") ADD(rep,"repeat")
  printf("%d: %s\n",n,k?out:"ok");}
 return 0;}
