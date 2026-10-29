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

static long ts[1100]; static char res[1100][8],src[1100][64];
int main(void){
  long W; int N; char line[512];
  if(!fgets(line,sizeof line,stdin)||sscanf(line,"%ld %d",&W,&N)!=2)return 0;
  static char al[64][64]; int na=0;
  while(fgets(line,sizeof line,stdin)){ line[strcspn(line,"\n")]=0; if(!*line)break; if(na<64&&!strncmp(line,"ALLOW ",6))snprintf(al[na++],64,"%s",line+6); }
  int n=0; char user[64];
  while(n<1100&&fgets(line,sizeof line,stdin)) if(sscanf(line,"%ld %7s %63s %63s",&ts[n],res[n],user,src[n])==4)n++;
  static char sk[100][64]; static long su[100]; int nk=0,any=0;
  for(int e=0;e<n;e++){
    if(strcmp(res[e],"fail"))continue;
    int al_hit=0; for(int i=0;i<na;i++) if(glob_match(al[i],src[e]))al_hit=1;
    if(al_hit)continue;
    int cnt=0; for(int i=e;i>=0;i--) if(!strcmp(res[i],"fail")&&!strcmp(src[i],src[e])&&ts[i]>ts[e]-W)cnt++;
    int ki=-1; for(int i=0;i<nk;i++) if(!strcmp(sk[i],src[e]))ki=i;
    if(ki<0){ki=nk++;strcpy(sk[ki],src[e]);su[ki]=-1;}
    if(cnt>=N&&ts[e]>=su[ki]){
      su[ki]=ts[e]+W; any=1;
      char fo[24]="none"; for(int j=e;j<n;j++) if(!strcmp(res[j],"ok")&&!strcmp(src[j],src[e])){snprintf(fo,24,"%ld",ts[j]);break;}
      printf("ALERT %ld %s failures=%d first-ok=%s\n",ts[e],src[e],cnt,fo);
    }
  }
  if(!any)puts("no alerts");
  return 0;
}
