#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/* Finds `key` on a key=value line (values may be "quoted \"with\" spaces"). First occurrence wins. */
static int get_field(const char *line, const char *key, char *out, size_t cap) {
  const char *p = line; size_t kl = strlen(key);
  while (*p) {
    while (*p == ' ') p++;
    if (!*p) break;
    const char *k = p;
    while (*p && *p != '=' && *p != ' ') p++;
    size_t n = (size_t)(p - k), vl = 0;
    char v[1024];
    if (*p == '=') {
      p++;
      if (*p == '"') {
        p++;
        while (*p && *p != '"') { if (*p == '\\' && p[1]) p++; if (vl < sizeof v - 1) v[vl++] = *p; p++; }
        if (*p == '"') p++;
      } else while (*p && *p != ' ') { if (vl < sizeof v - 1) v[vl++] = *p; p++; }
    }
    v[vl] = 0;
    if (n == kl && memcmp(k, key, n) == 0) { snprintf(out, cap, "%s", v); return 1; }
  }
  return 0;
}

static int selv[26]; static char toks[64][16]; static int nt, tp;
static int parse_or(void);
static int parse_not(void){
  if(tp<nt&&!strcmp(toks[tp],"not")){tp++;return !parse_not();}
  if(tp<nt&&!strcmp(toks[tp],"(")){tp++;int v=parse_or();if(tp<nt&&!strcmp(toks[tp],")"))tp++;return v;}
  int v=0; if(tp<nt){v=selv[toks[tp][0]-'a'];tp++;} return v;
}
static int parse_and(void){int v=parse_not();while(tp<nt&&!strcmp(toks[tp],"and")){tp++;int w=parse_not();v=v&&w;}return v;}
static int parse_or(void){int v=parse_and();while(tp<nt&&!strcmp(toks[tp],"or")){tp++;int w=parse_and();v=v||w;}return v;}
int main(void){
  char field[26][64],op[26][16],val[26][256]; int has[26]={0};
  char line[1024],cond[512]="";
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0;
    if(!*line)break;
    if(!strncmp(line,"COND ",5)){snprintf(cond,sizeof cond,"%s",line+5);continue;}
    char kw[16],nm[16],fl[64],o[16]; int off=0;
    if(sscanf(line,"%15s %15s %63s %15s %n",kw,nm,fl,o,&off)>=4){
      int i=nm[0]-'a'; has[i]=1; strcpy(field[i],fl); strcpy(op[i],o); snprintf(val[i],256,"%s",line+off);
    }
  }
  nt=0; for(char *t=strtok(cond," ");t&&nt<64;t=strtok(NULL," "))snprintf(toks[nt++],16,"%s",t);
  int ev=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; ev++;
    for(int i=0;i<26;i++){ selv[i]=0; if(!has[i])continue;
      char f[1024]; if(!get_field(line,field[i],f,sizeof f))continue;
      size_t fl=strlen(f),vl=strlen(val[i]);
      if(!strcmp(op[i],"eq"))selv[i]=!strcmp(f,val[i]);
      else if(!strcmp(op[i],"contains"))selv[i]=strstr(f,val[i])!=NULL;
      else if(!strcmp(op[i],"startswith"))selv[i]=strncmp(f,val[i],vl)==0;
      else if(!strcmp(op[i],"endswith"))selv[i]=fl>=vl&&strcmp(f+fl-vl,val[i])==0;
    }
    tp=0; printf("event %d: %s\n",ev,parse_or()?"match":"no match");
  }
  return 0;
}
