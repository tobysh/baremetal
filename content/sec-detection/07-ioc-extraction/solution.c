#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static char out[1200][80]; static int ot[1200]; static int no;
static void add(int t,const char *s){ for(int i=0;i<no;i++) if(ot[i]==t&&!strcmp(out[i],s))return; if(no<1200){ot[no]=t;snprintf(out[no],80,"%s",s);no++;} }
static int is_ip(const char *s){
  int parts=0; const char *p=s;
  while(1){
    int d=0,v=0; const char *st=p;
    while(*p>='0'&&*p<='9'){v=v*10+(*p-'0');d++;p++; if(d>3)return 0;}
    if(d==0||v>255||(d>1&&*st=='0'))return 0;
    parts++;
    if(*p=='.'){p++;continue;}
    break;
  }
  return parts==4&&*p==0;
}
static int is_domain(const char *s){
  size_t n=strlen(s); if(n<6||strcmp(s+n-5,".test"))return 0;
  size_t i=0; int labels=0;
  while(i<=n){
    size_t j=i; while(s[j]&&s[j]!='.')j++;
    if(j==i)return 0; if(s[i]=='-'||s[j-1]=='-')return 0;
    for(size_t k=i;k<j;k++) if(!((s[k]>='a'&&s[k]<='z')||(s[k]>='0'&&s[k]<='9')||s[k]=='-'))return 0;
    labels++; if(!s[j])break; i=j+1;
  }
  return labels>=2;
}
static int cmp(const void *a,const void *b){ int x=*(const int*)a,y=*(const int*)b; if(ot[x]!=ot[y])return ot[x]-ot[y]; return strcmp(out[x],out[y]); }
int main(void){
  char line[1024];
  while(fgets(line,sizeof line,stdin)){
    char f[1024]; size_t m=0;
    for(char *p=line;*p&&*p!='\n';){
      if(!strncmp(p,"[.]",3)||!strncmp(p,"(.)",3)){f[m++]='.';p+=3;} else f[m++]=*p++;
    }
    f[m]=0;
    char *p=f;
    while(*p){
      while(*p&&!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'))p++;
      char tok[300]; size_t n=0;
      while(*p&&((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-')){ if(n<299)tok[n++]=*p; p++; }
      tok[n]=0;
      char *s=tok; while(*s=='.')s++;
      size_t l=strlen(s); while(l&&s[l-1]=='.')s[--l]=0;
      if(!*s)continue;
      for(char *c=s;*c;c++) if(*c>='A'&&*c<='Z')*c+=32;
      int hex=1; for(char *c=s;*c;c++) if(!((*c>='0'&&*c<='9')||(*c>='a'&&*c<='f')))hex=0;
      if(hex&&(l==32||l==64))add(2,s);
      else if(is_ip(s))add(0,s);
      else if(is_domain(s))add(1,s);
    }
  }
  if(!no){puts("no indicators");return 0;}
  int idx[1200]; for(int i=0;i<no;i++)idx[i]=i;
  qsort(idx,no,sizeof(int),cmp);
  const char *nm[3]={"ip","domain","hash"};
  for(int i=0;i<no;i++)printf("%s %s\n",nm[ot[idx[i]]],out[idx[i]]);
  return 0;
}
