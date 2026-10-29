#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int main(void){
  const char *keys[3]={"user","src","action"};
  char line[1024];
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0;
    char v[3][1024]; int have[3];
    for(int i=0;i<3;i++) have[i]=0;
    const char *p=line;
    while(*p){
      while(*p==' ')p++;
      if(!*p)break;
      const char *k=p; while(*p&&*p!='='&&*p!=' ')p++;
      size_t kl=(size_t)(p-k), vl=0; char val[1024];
      if(*p=='='){ p++;
        if(*p=='"'){ p++; while(*p&&*p!='"'){ if(*p=='\\'&&p[1])p++; val[vl++]=*p; p++; } if(*p=='"')p++; }
        else while(*p&&*p!=' '){ val[vl++]=*p; p++; }
      }
      val[vl]=0;
      for(int i=0;i<3;i++) if(!have[i]&&strlen(keys[i])==kl&&memcmp(k,keys[i],kl)==0){have[i]=1;strcpy(v[i],val);}
    }
    printf("user=%s src=%s action=%s\n",have[0]?v[0]:"-",have[1]?v[1]:"-",have[2]?v[2]:"-");
  }
  return 0;
}
