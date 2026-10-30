#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/* Safe local redirect: one leading slash, no backslash, no control or space bytes (raw or %-escaped). */
int safe_redirect(const char *t){
  if(!t)return 0; size_t n=strlen(t); if(n==0||n>2048||t[0]!='/')return 0;
  for(size_t i=0;i<n;i++){
    unsigned c=(unsigned char)t[i];
    if(c<=0x20||c==0x7f||c=='\\')return 0;
    if(c=='%'){
      if(i+2>=n)return 0;
      int a,b; unsigned char x=(unsigned char)t[i+1],y=(unsigned char)t[i+2];
      a=(x>='0'&&x<='9')?x-'0':(x>='a'&&x<='f')?x-'a'+10:(x>='A'&&x<='F')?x-'A'+10:-1;
      b=(y>='0'&&y<='9')?y-'0':(y>='a'&&y<='f')?y-'a'+10:(y>='A'&&y<='F')?y-'A'+10:-1;
      if(a<0||b<0)return 0;
      unsigned v=(unsigned)(a*16+b);
      if(v<0x20||v==0x7f||v==0x5c)return 0;
      if(i==1&&v==0x2f)return 0;
    }
  }
  if(n>=2&&(t[1]=='/'))return 0;
  return 1;
}

typedef struct { char sid[40],tok[40]; int live; long created,last; } S;
static S s[1100];
int main(void){
  long idle,ab; char site[128]; if(scanf("%ld %ld %127s",&idle,&ab,site)!=3)return 0;
  char ln[512]; fgets(ln,sizeof ln,stdin); int ns=0,n=0;
  while(fgets(ln,sizeof ln,stdin)){
    long t; char act[16],sid[64],a[128],b[64],c[300];
    if(sscanf(ln,"%ld %15s %63s",&t,act,sid)!=3)continue;
    n++; int k=-1; for(int i=0;i<ns;i++) if(!strcmp(s[i].sid,sid))k=i;
    char res[400];
    if(!strcmp(act,"login")){
      sscanf(ln,"%ld %15s %63s %63s",&t,act,sid,b);
      if(k>=0)strcpy(res,"rejected-fixation"); else if(ns<1100){ snprintf(s[ns].sid,40,"%s",sid); snprintf(s[ns].tok,40,"%s",b); s[ns].live=1; s[ns].created=s[ns].last=t; ns++; strcpy(res,"ok"); } else strcpy(res,"rejected-fixation");
    } else {
      sscanf(ln,"%ld %15s %63s %127s %63s %299s",&t,act,sid,a,b,c);
      if(k<0||!s[k].live)strcpy(res,"deny:no-session");
      else if(t-s[k].created>ab){s[k].live=0;strcpy(res,"deny:expired-absolute");}
      else if(t-s[k].last>idle){s[k].live=0;strcpy(res,"deny:expired-idle");}
      else if(strcmp(a,site))strcpy(res,"deny:cross-origin");
      else if(!strcmp(b,"-")||strcmp(b,s[k].tok))strcpy(res,"deny:bad-token");
      else { s[k].last=t; snprintf(res,sizeof res,"allow redirect=%s",safe_redirect(c)?c:"/"); }
    }
    printf("%d: %s\n",n,res);
  }
  return 0;
}
