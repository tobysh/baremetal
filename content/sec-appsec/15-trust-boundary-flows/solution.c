#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static int zl(const char *z){ return !strcmp(z,"internet")?0:!strcmp(z,"dmz")?1:!strcmp(z,"internal")?2:!strcmp(z,"admin")?3:-1; }
int main(void){
  static char nm[60][40]; static int nz[60]; int nn=0; char line[512];
  static char fl[220][512]; int nf=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0;
    if(!strncmp(line,"NODE ",5)&&nn<60){ char z[32]; if(sscanf(line+5,"%39s %31s",nm[nn],z)==2){nz[nn]=zl(z);nn++;} }
    else if(!strncmp(line,"FLOW ",5)&&nf<220)strcpy(fl[nf++],line+5);
  }
  int total=0;
  for(int i=0;i<nf;i++){
    char id[40],s[40],d[40],e[16],a[16],dt[16];
    if(sscanf(fl[i],"%39s %39s %39s enc=%15s auth=%15s data=%15s",id,s,d,e,a,dt)!=6)continue;
    int si=-1,di=-1; for(int k=0;k<nn;k++){ if(!strcmp(nm[k],s))si=k; if(!strcmp(nm[k],d))di=k; }
    if(si<0){printf("%s: error unknown node %s\n",id,s);continue;}
    if(di<0){printf("%s: error unknown node %s\n",id,d);continue;}
    int sl=nz[si],dl=nz[di]; char out[200]=""; int c=0;
    int pub=!strcmp(dt,"public");
    if(!strcmp(e,"no")&&(sl!=dl||!pub)){ c+=snprintf(out+c,sizeof out-c,"%sno-encryption",c?",":""); total++; }
    if(!strcmp(a,"no")&&sl<dl){ c+=snprintf(out+c,sizeof out-c,"%sno-authentication",c?",":""); total++; }
    if(!strcmp(dt,"secret")&&dl<2){ c+=snprintf(out+c,sizeof out-c,"%ssecret-exposed",c?",":""); total++; }
    if(sl==0&&dl>=2){ c+=snprintf(out+c,sizeof out-c,"%sskips-dmz",c?",":""); total++; }
    if(c)printf("%s: %s\n",id,out);
  }
  printf("findings=%d flows=%d\n",total,nf);
  return 0;
}
