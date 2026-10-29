#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static int glob_match(const char *p, const char *s) {
  if (!*p) return !*s;
  if (*p == '*') { for (;; s++) { if (glob_match(p + 1, s)) return 1; if (!*s) return 0; } }
  if (!*s) return 0;
  if (*p == '?' || *p == *s) return glob_match(p + 1, s + 1);
  return 0;
}

int main(void){
  static char ar[64][3][96]; int na=0; char line[512];
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; if(!*line)break;
    char kw[16]; if(na<64&&sscanf(line,"%15s %95s %95s %95s",kw,ar[na][0],ar[na][1],ar[na][2])==4)na++;
  }
  int hits[64]={0}; static char kept[600][300]; int nk=0,sup=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; char a[96],b[96],c[96];
    if(sscanf(line,"%95s %95s %95s",a,b,c)!=3)continue;
    int hit=-1; for(int i=0;i<na&&hit<0;i++) if(glob_match(ar[i][0],a)&&glob_match(ar[i][1],b)&&glob_match(ar[i][2],c))hit=i;
    if(hit>=0){hits[hit]++;sup++;} else snprintf(kept[nk++],300,"kept: %s %s %s",a,b,c);
  }
  printf("kept=%d suppressed=%d\n",nk,sup);
  for(int i=0;i<na;i++)printf("allow %d hits=%d%s\n",i+1,hits[i],hits[i]?"":" stale");
  for(int i=0;i<nk;i++)puts(kept[i]);
  return 0;
}
