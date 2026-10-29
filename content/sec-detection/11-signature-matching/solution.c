#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static int hv(int c){ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1; }
static int dehex(const char *s, uint8_t *o, size_t cap, size_t *n){
  size_t l=strlen(s); if(l%2||l/2>cap)return 0;
  for(size_t i=0;i<l;i+=2){ int a=hv(s[i]),b=hv(s[i+1]); if(a<0||b<0)return 0; o[i/2]=(uint8_t)(a*16+b); }
  *n=l/2; return 1;
}
static int lc(int c){ return c>='A'&&c<='Z'?c+32:c; }
typedef struct { int sid; size_t off,depth; int nocase; uint8_t pat[256]; size_t pl; } Sig;
int main(void){
  Sig S[32]; int ns=0; char line[2048];
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; if(!*line)break;
    char kw[8],cs[16],hx[600]; Sig *g=&S[ns];
    if(ns<32&&sscanf(line,"%7s %d %zu %zu %15s %599s",kw,&g->sid,&g->off,&g->depth,cs,hx)==6&&dehex(hx,g->pat,256,&g->pl)){ g->nocase=!strcmp(cs,"nocase"); ns++; }
  }
  int n=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; n++;
    uint8_t b[600]; size_t bl;
    printf("payload %d: ",n);
    if(!dehex(line,b,600,&bl)){puts("invalid");continue;}
    int any=0;
    for(int i=0;i<ns;i++){
      Sig *g=&S[i]; size_t end=bl; if(g->depth&&g->off+g->depth<end)end=g->off+g->depth;
      int m=0;
      for(size_t st=g->off;!m&&st+g->pl<=end;st++){
        size_t k=0; while(k<g->pl&&(g->nocase?lc(b[st+k])==lc(g->pat[k]):b[st+k]==g->pat[k]))k++;
        if(k==g->pl)m=1;
      }
      if(m){printf("%s%d",any?" ":"",g->sid);any=1;}
    }
    puts(any?"":"-");
  }
  return 0;
}
