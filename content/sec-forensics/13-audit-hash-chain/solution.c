#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint64_t fnv(const char*s){uint64_t h=14695981039346656037ULL;for(;*s;s++){h^=(uint8_t)*s;h*=1099511628211ULL;}return h;}
int main(void){char line[1024];char prev[17]="0000000000000000";unsigned long exp=1;int ln=0;
 while(fgets(line,sizeof line,stdin)){ln++;size_t len=strlen(line);while(len&&(line[len-1]=='\n'||line[len-1]=='\r'))line[--len]=0;
  char*f[4];int nf=0;char*p=line;f[nf++]=p;
  while(*p){if(*p=='|'){*p=0;if(nf==4){nf=5;break;}f[nf++]=p+1;}p++;}
  int bad=nf!=4;
  if(!bad){size_t sl=strlen(f[0]);if(sl==0||sl>9||f[0][0]=='0')bad=1;for(size_t i=0;i<sl&&!bad;i++)if(f[0][i]<'0'||f[0][i]>'9')bad=1;
   if(!f[1][0]||!f[2][0])bad=1;
   if(strlen(f[3])!=16)bad=1;for(int i=0;i<16&&!bad;i++){char c=f[3][i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))bad=1;}}
  if(bad){printf("BROKEN at line %d: malformed\n",ln);return 0;}
  unsigned long seq=strtoul(f[0],NULL,10);
  if(seq!=exp){printf("BROKEN at line %d: sequence (expected %lu, found %lu)\n",ln,exp,seq);return 0;}
  char buf[1024];snprintf(buf,sizeof buf,"%s|%lu|%s|%s",prev,seq,f[1],f[2]);char want[17];snprintf(want,sizeof want,"%016llx",(unsigned long long)fnv(buf));
  if(strcmp(want,f[3])){printf("BROKEN at line %d: hash mismatch\n",ln);return 0;}
  strcpy(prev,f[3]);exp++;}
 printf("chain ok: %lu entries head=%s\n",exp-1,prev);return 0;}
