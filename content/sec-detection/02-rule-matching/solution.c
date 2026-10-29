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

int main(void){
  char name[32][64],field[32][64],op[32][16],val[32][256]; int nr=0;
  char line[1024];
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0;
    if(!*line)break;
    char kw[16]; int off=0;
    if(sscanf(line,"%15s %63s %63s %15s %n",kw,name[nr],field[nr],op[nr],&off)>=4){ snprintf(val[nr],256,"%s",line+off); nr++; }
  }
  int ev=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; ev++;
    printf("event %d: ",ev); int any=0;
    for(int i=0;i<nr;i++){
      char f[1024];
      if(!get_field(line,field[i],f,sizeof f))continue;
      size_t fl=strlen(f),vl=strlen(val[i]); int m=0;
      if(!strcmp(op[i],"eq"))m=!strcmp(f,val[i]);
      else if(!strcmp(op[i],"contains"))m=strstr(f,val[i])!=NULL;
      else if(!strcmp(op[i],"startswith"))m=strncmp(f,val[i],vl)==0;
      else if(!strcmp(op[i],"endswith"))m=fl>=vl&&strcmp(f+fl-vl,val[i])==0;
      if(m){printf("%s%s",any?",":"",name[i]);any=1;}
    }
    printf("%s\n",any?"":"-");
  }
  return 0;
}
