#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static char vn[64][32]; static int vm[64]; static int nv;
static int getm(const char *n){ for(int i=0;i<nv;i++) if(!strcmp(vn[i],n))return vm[i]; return 0; }
static void setm(const char *n,int m){ for(int i=0;i<nv;i++) if(!strcmp(vn[i],n)){vm[i]=m;return;} if(nv<64){snprintf(vn[nv],32,"%s",n);vm[nv++]=m;} }
static int arg_mask(const char *a){ if(!*a||a[0]=='"')return 0; return getm(a); }
static int operand(const char *o){
  if(o[0]=='"')return 0;
  const char *lp=strchr(o,'(');
  if(lp&&o[strlen(o)-1]==')'){
    char f[32],a[64]; size_t fl=(size_t)(lp-o); if(fl>31)fl=31; memcpy(f,o,fl); f[fl]=0;
    size_t al=strlen(lp+1)-1; if(al>63)al=63; memcpy(a,lp+1,al); a[al]=0;
    int m=arg_mask(a);
    if(!strcmp(f,"input")||!strcmp(f,"env"))return 3;
    if(!strcmp(f,"escape_sql"))return m&~1;
    if(!strcmp(f,"quote_shell"))return m&~2;
    return m;
  }
  return getm(o);
}
int main(void){
  char line[512]; int ln=0,any=0;
  while(fgets(line,sizeof line,stdin)){
    ln++; line[strcspn(line,"\n")]=0;
    char *s=line; while(*s==' '||*s=='\t')s++;
    if(!*s||*s=='#')continue;
    if(!strncmp(s,"sink_sql(",9)||!strncmp(s,"sink_exec(",10)){
      int sql=!strncmp(s,"sink_sql(",9); char *lp=strchr(s,'('); char *rp=strrchr(s,')'); if(!rp)continue;
      *rp=0; const char *a=lp+1; int m=arg_mask(a);
      printf("line %d: %s(%s) %s\n",ln,sql?"sink_sql":"sink_exec",a,(m&(sql?1:2))?"TAINTED":"clean"); any=1; continue;
    }
    char *eq=strstr(s," = "); if(!eq)continue;
    char name[32]; size_t nl=(size_t)(eq-s); if(nl>31)nl=31; memcpy(name,s,nl); name[nl]=0;
    char *rest=eq+3; int m=0;
    while(1){
      char *plus=strstr(rest," + "); if(plus)*plus=0;
      m|=operand(rest);
      if(!plus)break; rest=plus+3;
    }
    setm(name,m);
  }
  if(!any)puts("no sinks");
  return 0;
}
